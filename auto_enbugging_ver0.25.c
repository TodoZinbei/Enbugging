/*
  Automatic Enbugging Program

  by Takeru "TODO" Zinbei MIYAZAKI

  start       2014/01/16
  ver. 0.25   2014/02/10 



  ver. 0.10   2014/01/16  固定の置換テーブルによる文字の置き換えエラー実装
  ver. 0.11   2014/01/16  置換テーブルをファイルから読み込む機能を追加
  ver. 0.12   2014/01/16  ==を=と間違えるという基本的なエラーを追加
  ver. 0.13   2014/01/17  {に対応する}が欠落する、と、括弧の種類が違うというエラーを追加
  ver. 0.14   2014/01/17  エンバグする個数の指定と、エンバグ個所の表示On/Offの切り替え,ヘルプ表示追加
  ver. 0.15   2014/01/20  ;の削除と挿入を追加
  ver. 0.16   2014/01/27  ()内部の;を,に変更するエラーを追加(主にfor()文)
  ver. 0.17   2014/01/28  bug埋め込み試行回数にリミット搭載
  ver. 0.18   2014/01/29  何も引数を与えない時に、対話モードで動く機能追加。
  ver. 0.19   2014/01/30  対話モードの際に 入力ソースを一部表示する機能追加。
                          表示する行番号を一般的な0から1に修正。
  ver. 0.20   2014/01/31  括弧置換の抑制、 <=などの入れ替え
  ver. 0.21   2014/02/03  エンバグルーチン、毎回ランダムに選んだ場所にランダムに選んだバグを埋め込めるかテスト
                          していたものを、場所だけ毎回ランダムで、バグの種類は選んだものを埋め込み上限までテスト
                          するように変更。将来のバグ出現頻度を調節できるようにするための下準備。
  ver. 0.22   2014/02/04  引数2つの場合、もし2つ目の引数がハイフンか数字であったら出力ファイル名ではなく
                          埋め込むバグの個数指定と判断する部分を追加(ハイフンで始まるファイルを作らないため)
  ver. 0.23   2014/02/06  %{d,f}と%sの入れ替え
                          %8.4fのような%とfなどの間に桁数指定する部分を正しく
                          判定できるように修正。
  ver. 0.24   2014/02/07  何文字目にエンバグしたかを示すヒントが先頭を0文字目に
                          なっているのを1文字目になるよう修正
  ver. 0.25   2014/02/10  連続するアルファベットを入れ替える処理を追加
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 入力バッファサイズ */
#define BUFSIZE 1024

/* 入力プログラムの最大行数 */
#define MAXLINE 65536

/* 埋め込み試行上限 */
/* ver.0.20までは、ランダムのエンバグ位置探索上限数、ver.0.21から1つ選択されたバグの種類を埋め込む場所を探す回数の上限*/
#define MAXENBUGTRIAL 100000

/* 埋め込みバグモード数 */
#define ENBUGTYPES 8

/* 埋め込みバグのモードを常にモード数-1にするかどうかのフラグ */
#define NEWENBUGONLY 0

#define REPLACEGROUP 16
#define REPLACELEN 16
#define REPLACETABLEFILENAME "ReplaceTable.txt"

struct RepTable {
  int groupnum;
  char group[REPLACEGROUP][REPLACELEN];
  int charanum[REPLACEGROUP];
};

//#define Bracketsnum 4
//char Brackets[2][Bracketsnum+1]={"{[(<","}])>"};
/* ver.0.20 括弧置換の種類数抑制 */
#define Bracketsnum 2
char Brackets[2][Bracketsnum+1]={"{(","})"};


void Delete_remark(char**,int);
void Enbug(char**,char **,int,int,struct RepTable *);
char* Insert_character(char *,int,char);
char* Delete_character(char *,int);
void swapchar(char *, char *);

int main(int argc, char *argv[]){

  FILE *input_fp, *output_fp;
  char buf[BUFSIZE];   /* 読み込みバッファ */
  char **prog;         /* プログラム格納用文字列の配列ポインタ */
  char **noremprog;    /* 注釈文を省いたプログラムの格納用ポインタ */
  int line;            /* 読み込んだプログラムの行数 */
  int i,j;
  int numoferr=1;        /* エンバグする個数と動作モード指定 */
  struct RepTable r_table; /* 置換テーブル */

  /* help */
  if ( argc > 1 ) {
    if ( strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0 ) {

      fprintf(stderr, "Enbugger : Automatic Enbugging for Source Codes\n");
      fprintf(stderr, " usage > %s <source file> <output file> <number of error(s)>\n",argv[0]);
      fprintf(stderr, " <source file> : an input file name of the original source code  ( Default : standard-in  )\n");
      fprintf(stderr, " <output file> : an output file name of the enbugged source code ( Default : standard-out )\n");
      fprintf(stderr, " <number of error(s)> : a number of times for the enbugging\n");
      fprintf(stderr, "                        > 0 ... normal mode\n");
      fprintf(stderr, "                        = 0 ... output no-remark program mode\n");
      fprintf(stderr, "                        < 0 ... enbugging with hints mode\n");
      exit(0);
    }

  }

  /* rand()関数のシード初期化 */
  srand( (unsigned int)time(0));


  /* 置換テーブルの初期化 */
  /* 置換テーブルファイルが読み込めなかった場合の固定置換テーブル */
  r_table.groupnum = 2;
  strcpy(r_table.group[0],"ijl1");
  r_table.charanum[0]=strlen(r_table.group[0]);
  strcpy(r_table.group[1],"nm");
  r_table.charanum[1]=strlen(r_table.group[1]);
  
  /* 置換テーブルファイルの読み込み  */
  if ((input_fp=fopen(REPLACETABLEFILENAME,"r"))!=NULL){

    for(i=0;i<REPLACEGROUP;i++){
      if (fgets(buf,sizeof(buf),input_fp)==NULL) {
	break;
      }
      buf[strlen(buf)-1]='\0';  // return --> null
      strncpy(r_table.group[i],buf,REPLACELEN-1);
      r_table.group[i][REPLACELEN-1] = '\0';
      r_table.charanum[i]=strlen(r_table.group[i]);
      if (r_table.charanum[i] < 2) { 
        /* 置換グループには最低2文字必要→1文字以下しか存在しない行は読飛ばす */
	i--;
      } else {
	fprintf(stderr,"replace %d : %s (#=%d)\n",i,r_table.group[i],r_table.charanum[i]);
      }
    }
    r_table.groupnum=i;
    fprintf(stderr,"# of replace groups = %d\n",r_table.groupnum);
    fclose(input_fp);
  } else {
    fprintf(stderr, "Cannot open a replace table file %s.\n",REPLACETABLEFILENAME);
    fprintf(stderr, "--> Using default replace table.\n");
  }


  // File Open

  // Default
  /* エンバグしたプログラムを標準出力へ出力する  */
  output_fp=stdout;
  /* 変換する元のプログラムを標準入力から入力する  */
  input_fp=stdin;

  if (argc > 1) {

    if ( strcmp(argv[1],"-")!=0 && strcmp(argv[1],"stdin")!=0) {
      /* 入力ファイルを開く */
      if ((input_fp=fopen(argv[1],"r"))==NULL) {
	fprintf(stderr, "ERROR! Cannot open file : %s\n",argv[1]);
	exit(1);
      }
    
    }

    if (argc > 2) {
	
      // ver. 0.22
      if (argc == 3 && (argv[2][0]=='-' || isdigit(argv[2][0])!=0 )) {
	fprintf(stderr, "input-file and number mode\n");
	
	sscanf(argv[2],"%d",&numoferr);
	fprintf(stderr, "Number of error = %d ",numoferr);
	
	if (numoferr > 0) {
	  fprintf(stderr, "(normal mode)\n");
	} else if (numoferr == 0) {
	  fprintf(stderr, "(output no-remark program mode)\n");
	} else {
	  fprintf(stderr, "(enbugging with hints mode)\n");
	}


      } else {

	if ( strcmp(argv[2],"-")!=0 && strcmp(argv[2],"stdout")!=0) {
	  /* 出力ファイルを開く */
	  if ((output_fp=fopen(argv[2],"w"))==NULL) {
	    fprintf(stderr, "ERROR! Cannot open file : %s\n",argv[2]);
	    exit(1);
	  }
	}      
	
	if (argc > 3) {
	  
	  sscanf(argv[3],"%d",&numoferr);
	  fprintf(stderr, "Number of error = %d ",numoferr);
	  
	  if (numoferr > 0) {
	    fprintf(stderr, "(normal mode)\n");
	  } else if (numoferr == 0) {
	    fprintf(stderr, "(output no-remark program mode)\n");
	  } else {
	    fprintf(stderr, "(enbugging with hints mode)\n");
	  }
	}
      
      }

    } 
    //  else {
    //    }

  }
  else {
    /* 対話モード */
    printf("<< Interactive mode >>\n");

    while(1){
      printf("> Input file name ? : ");
      scanf("%s",buf);
      if ((input_fp=fopen(buf,"r"))!=NULL) {
	break;
      }
      printf("ERROR! Cannot open file : %s\n",buf);	
    }

    while(1){
      printf("> Output file name ? ( please input - if you want to output standard output) : ");
      scanf("%s",buf);
      if (strcmp(buf,"-")==0) {
	break;
      }
      if ((output_fp=fopen(buf,"w"))!=NULL) {
	break;
      }
      printf("ERROR! Cannot open file : %s\n",buf);	
    }
     
    while(1){
      printf("> How many do you want to enbug? : ");
      scanf("%d",&numoferr);
      if (numoferr > 0) break;
      printf("ERROR! Please input positive number!\n");	
    }
     
    while(1){
      printf("> Do you want to confirm the enbugged place(s) ? (y/n) : ");
      scanf("%s",buf);
      if (strcmp(buf,"y")==0) {
	numoferr*=-1;
	break;
      }
      if (strcmp(buf,"n")==0) {
	break;
      }
      printf("ERROR! Please input y or n!\n");	
    }

  }

  


  // Read the Original Program
  /* エンバグ処理のため、同じプログラムを2つの配列prog[],noremprog[]に読み込む */
  prog=malloc(sizeof(char *)*MAXLINE);
  noremprog=malloc(sizeof(char *)*MAXLINE);
  line=0;
  while(fgets(buf,sizeof(buf),input_fp)!=NULL) {
    //   fprintf(stderr,"read %s : strlen : %d\n",buf,(int)strlen(buf));    
    prog[line]=(char *)calloc(strlen(buf)+1,sizeof(char));
    noremprog[line]=(char *)calloc(strlen(buf)+1,sizeof(char));
    strncpy(prog[line],buf,strlen(buf));
    strncpy(noremprog[line],buf,strlen(buf));
    //   fprintf(stderr,"%4d : %s\n",line,prog[line]);
    if (argc <= 1 && line<10) {
      printf("%4d : %s",line+1,prog[line]);
    }
    line++;
  }


  // delete remarks
  Delete_remark(noremprog,line);

  /* output no-remark program (numoferr = 0 only) */

  if (numoferr == 0) {
    for(i=0;i<line;i++){
      fprintf(output_fp,"%s",noremprog[i]);
    }
  } else {
    // Enbugging!
    Enbug(prog,noremprog,line,numoferr,&r_table);
    

    // Output Enbagged program
    for(i=0;i<line;i++) {
      fputs(prog[i],output_fp);
    }
  }
  
  // Memory Free
  for(i=0;i<line;i++){
    free(prog[i]);
    free(noremprog[i]);
  }
  free(prog);
  free(noremprog);

  // File Close
  if (output_fp!=stdout) fclose(output_fp);
  if (input_fp!=stdin) fclose(input_fp);

  /* test fot Insert and Delete characters functions
     strcpy(buf,"ABC");
     printf("Insert test %s\n",Insert_character(buf,1,'D'));
     printf("Delete test %s\n",Delete_character(buf,1));
  */

  return 0;
}


void Delete_remark(char **nrp,int l){

  /*
    読み込んだプログラムから、注釈文と文字列"..."の記号以外を取り除く
    (エンバグしてもコンパイルエラーにならない/なりにくい部分はエンバグする対象から外すため)
  */

  int flag=0;    /* 状態 =1 ... 文末まで注釈、=2 ... 注釈／＊...＊／の内部、=3 ... 文字列の内部 */
  int i,j,length;

  for(i=0;i<l;i++){

    for(j=0;j<(strlen(nrp[i])-1);j++) {

      switch(flag){

      case 1 :
	nrp[i][j]=' ';
	break;
      case 2 :
	if (nrp[i][j]=='*' && nrp[i][j+1]=='/') {
	  nrp[i][j+1]=' ';
	  flag=0;
	}
	nrp[i][j]=' ';
	break;
      case 3 :  // Inner "..."
	if (nrp[i][j]=='\"') {
	  if(j==0 || (j>0 && nrp[i][j-1]!='\\')){
	    flag=0;
	  }
	  break;
	} else {
	  if (nrp[i][j]=='\\'){
	    j++;  // skip it and its following character
	    break;
	  } else {
	    if (nrp[i][j]=='%'){
	      do {
		j++;
	      } while((nrp[i][j]=='.')||((nrp[i][j]>='0')&&(nrp[i][j]<='9')));
	      break;
	    }
	  }
	}
	nrp[i][j]=' ';
	break;
      case 0 :
	if (nrp[i][j]=='/') {
	  if(nrp[i][j+1]=='*') {
	    nrp[i][j]=' ';
	    flag=2;
	  } else {

	    if(nrp[i][j+1]=='/') {
	      nrp[i][j]=' ';
	      flag=1;
	    }
	  }
	}

	if(nrp[i][j]=='\"') {
	  if(j==0 || (j>0 && nrp[i][j-1]!='\\')){
	    flag=3;
	  }
	}
      }
    }
    /* 文末なのでもしflag=1なら=0(通常状態)に戻す */
    if (flag==1) {
      flag=0;
    }
  }

}


void Enbug(char **prog,char **nrp,int line,int nob,struct RepTable *rt){
  int count,trial,conerr;
  int c,i,j,r,s,k,mode;
  int modecount[ENBUGTYPES];
  for(i=0;i<ENBUGTYPES;i++) modecount[i]=0;
  conerr=0;

  for(count=0;count<abs(nob);count++) {

    mode=rand()%ENBUGTYPES;  // Choose Enbugging mode
    if (NEWENBUGONLY != 0) {
      mode=ENBUGTYPES-1;   //Test Mode for new enbug-mode
    }
    modecount[mode]++;

    //    fprintf(stderr,"Bug #%04d : mode %d\n",count+1,mode);

    for(trial=0;trial<MAXENBUGTRIAL;trial++){

      //      fprintf(stderr,"trial %d\n",trial);

      c=rand()%line;  // Choose random line
      j=rand()%(strlen(nrp[c])); // Choose random character
    
      switch(mode){
      case 0:
	// Replace by using Replace-table
	for(r=0;r<rt->groupnum;r++) {
	
	  for(i=0;i<rt->charanum[r];i++) {

	    if(nrp[c][j] == rt->group[r][i]) {
	    
	      s = (i + rand()%((rt->charanum[r]) - 1)+1)%(rt->charanum[r]);
	    
	      prog[c][j] = rt->group[r][s];
	    
	      if (nob < 0)
		fprintf(stderr,"Bug #%04d : replace %c into %c in line %d (%d)\n",count+1,nrp[c][j],prog[c][j],c+1,j+1);
	      nrp[c][j]=' '; // it has already been replaced
	      trial=MAXENBUGTRIAL+1;
	      r=rt->groupnum;
	      i=rt->charanum[r];
	    }
	  }
	
	}
	break;


      case 1:
	// Change == into = (one of the most frequently error)
	if(nrp[c][j] == '=') {
	
	  if ((j > 0 && nrp[c][j-1]=='=') || nrp[c][j+1]=='=' ) {
	  
	    if (nob < 0)
	      fprintf(stderr,"Bug #%04d : replace == into = in line %d (%d)\n",count+1,c+1,j+1);
	  
	    /*
	      prog[c][j]=' ';
	      nrp[c][j]=' '; 
	    */
	    Delete_character(prog[c],j);
	    Delete_character(nrp[c] ,j);
	    trial=MAXENBUGTRIAL+1;
	  } else {

	    if (j > 0 && prog[c][j] == '=' &&
		(nrp[c][j-1] == '>' || nrp[c][j-1] == '<' || nrp[c][j-1] == '!') ) {
	    
	      prog[c][j]=prog[c][j-1];
	      prog[c][j-1]='=';

	      if (nob < 0)
		fprintf(stderr,"Bug #%04d : replace %c= into =%c in line %d (%d)\n",count+1,nrp[c][j-1],nrp[c][j-1],c+1,j);

	      nrp[c][j-1]=nrp[c][j]=' '; // it has already been replaced
	      trial=MAXENBUGTRIAL+1;
	    
	    
	    } 
	  }
	  
	}
	break;
      
      case 2:
	// Lost } corresponding to {
	if(nrp[c][j] == '}') {
	
	  if (nob < 0)
	    fprintf(stderr,"Bug #%04d : delete } in line %d (%d)\n",count+1,c+1,j+1);
	  /*
	    prog[c][j]=' ';
	    nrp[c][j]=' ';
	  */
	  Delete_character(prog[c],j);
	  Delete_character(nrp[c] ,j);
      
	  trial=MAXENBUGTRIAL+1;
      
	}
	break;

      case 3:
	// Replace bracket-pair
	for(i=0;i<Bracketsnum;i++) {
	
	  if(nrp[c][j] == Brackets[0][i]) {
	  
	    s=0;
	    for(r=j+1;r<strlen(nrp[c]);r++){
	      if(nrp[c][r] == Brackets[1][i] && s==0) break;
	    
	      for(k=0;k<Bracketsnum;k++){
		if(nrp[c][r] == Brackets[0][k]) s++;
		if(nrp[c][r] == Brackets[1][k]) s--;
	      }
	    
	    
	    }
	  
	    if (r<strlen(nrp[c])) {
	    
	      s = (i + rand()%(Bracketsnum - 1)+1)%Bracketsnum;
	    
	      prog[c][j] = Brackets[0][s];
	      prog[c][r] = Brackets[1][s];
	    
	      if (nob < 0)	  
		fprintf(stderr,"Bug #%04d : replace %c...%c into %c...%c in line %d (%d...%d)\n",count+1,nrp[c][j],nrp[c][r],prog[c][j],prog[c][r],c+1,j+1,r+1);
	    
	      nrp[c][j]=' '; // it has already been replaced
	      nrp[c][r]=' '; 
	      trial=MAXENBUGTRIAL+1;
	      break;
	    }
	  }
	}
	break;

      case 4:
	// Add/Delete ; on the bottom of line

	r=0;  /* flag : r=1...行末に;がある、r=2...行末に;がない */
	for(j=strlen(nrp[c])-2;j>=0;j--){
	  if( nrp[c][j]==';' ) {
	    r=1;
	    break;
	  }
	  if( nrp[c][j]=='/' ) {
	    break;
	  }
	  if( nrp[c][j]!=' ' ) {
	    if (nrp[c][j]==')') {
	      r=2;
	    }
	    break;
	  }
	}
      
      
	if(r==1 && prog[c][j]==';' ) {
	  if (nob < 0)
	    fprintf(stderr,"Bug #%04d : delete ; in line %d (%d)\n",count+1,c+1,j+1);
	
	  Delete_character(prog[c],j);
	  //	Delete_character(nrp[c] ,j);
	
	  trial=MAXENBUGTRIAL+1;
	} else if (r==2 && prog[c][j+1]!=';') {
	
	  if (nob < 0)
	    fprintf(stderr,"Bug #%04d : insert ; in line %d (%d)\n",count+1,c+1,j+1);
	
	  Insert_character(prog[c],j+1,';');
	  Insert_character(nrp[c] ,j+1,' ');
	
	  trial=MAXENBUGTRIAL+1;
	}      
	break;

      case 5:
	// Replace ; to , between ( and )
	r=0;  /* flag : r>0...()内部、r=0...()内部じゃない */
	for(j=0;j<strlen(nrp[c])-1;j++){
	  if (nrp[c][j]=='(') r++;
	  if (nrp[c][j]==')') r--;

	  if ((r>0)&&(nrp[c][j]==';')) {
	    prog[c][j] = ',';
	  
	    if (nob < 0)
	      fprintf(stderr,"Bug #%04d : replace %c into %c in line %d (%d)\n",count+1,nrp[c][j],prog[c][j],c+1,j+1);
	    nrp[c][j]=' '; // it has already been replaced
	    trial=MAXENBUGTRIAL+1;
	    break;
	  }
	
	}
	break;

      case 6:
	// Switch %d or %f to %s 
	for(r=0;r<strlen(nrp[c])-1;r++){

	  if (nrp[c][r]=='%') {

	    for(s=r+1;s<strlen(nrp[c]);s++){
	      // fprintf(stderr,"%c %c\n",nrp[c][s],prog[c][s]);
	      if ((nrp[c][s]!='.')&&((nrp[c][s]<'0')||(nrp[c][s]>'9'))) break;
	    }
	    
	    if ((nrp[c][s]=='d')||(nrp[c][s]=='f')) {
	      prog[c][s]='s';
	      if (nob < 0) {
		fprintf(stderr,"Bug #%04d : replace %c into %c in line %d (%d)\n",count+1,nrp[c][s],prog[c][s],c+1,s+1);
	      }
	      nrp[c][s]=' '; // it has already been replaced
	      trial=MAXENBUGTRIAL+1;
	      break;
	    }

	    if (nrp[c][s]=='s') {
	      if (j%2) {
		prog[c][s]='d';
	      } else {
		prog[c][s]='f';
	      }
	      if (nob < 0) {
		fprintf(stderr,"Bug #%04d : replace %c into %c in line %d (%d)\n",count+1,nrp[c][s],prog[c][s],c+1,s+1);
	      }
	      nrp[c][s]=' '; // it has already been replaced
	      trial=MAXENBUGTRIAL+1;
	      break;
	    }
	  }
	
	}
	break;

      case 7:
	// Swap 2 aplhabets 

	if (isalpha(nrp[c][j])!=0) {
	  
	  if (isalpha(nrp[c][j+1])!=0 && nrp[c][j]==prog[c][j] && nrp[c][j+1]==prog[c][j+1] ) {
	    
	    swapchar(&(prog[c][j]),&(prog[c][j+1]));
	    
	    if (nob < 0) {
	      fprintf(stderr,"Bug #%04d : swap %c and %c in line %d (%d)\n",count+1,nrp[c][j],prog[c][j],c+1,j+1);
	    }
	    nrp[c][j]=nrp[c][j+1]=' '; // it has already been replaced
	    trial=MAXENBUGTRIAL+1;
	    break;
	  }
	  
	  if (j>0 && isalpha(nrp[c][j-1])!=0 && nrp[c][j]==prog[c][j] && nrp[c][j-1]==prog[c][j-1] ) {
	    
	    swapchar(&(prog[c][j]),&(prog[c][j-1]));
	    
	    if (nob < 0) {
	      fprintf(stderr,"Bug #%04d : swap %c and %c in line %d (%d)\n",count+1,nrp[c][j-1],prog[c][j-1],c+1,j);
	    }
	    nrp[c][j]=nrp[c][j-1]=' '; // it has already been replaced
	    trial=MAXENBUGTRIAL+1;
	    break;
	  }
	 
	
	}
	break;
	
	
      }
      
    }
    
    if (trial == MAXENBUGTRIAL) {
      count--;
      conerr++;
    } else {
      conerr=0;
    }
    
    if (conerr>=10) {
      fprintf(stderr,"We cannot do more enbugging for this probgram, since 10 times enbugging-trial fails continuously.\n",MAXENBUGTRIAL);
      break;
    }

    // for(i=0;i<ENBUGTYPES;i++) fprintf(stderr,"Mode %2d: trial : %6d\n",i,modecount[i]);
    
  }
}

char* Insert_character(char *str,int pos,char chr){

  /* str..挿入する文字列 */
  /* pos..挿入場所  posは0からstrlen(str)まで */
  /* chr..挿入する文字*/

  int i;

  //  fprintf(stderr,"Insert str: %s pos: %d char: %c\n",str,pos,chr);

  if (strlen(str)>=pos && pos >= 0){
    /* str[pos],str[pos+1],...,str[strlen(str)]を１つずつ後ろにずらす  */
    for(i=strlen(str);i>=pos;i--) {
      //      fprintf(stderr,"copy str[%d](%c)-->str[%d](%c)\n",i,str[i],i+1,str[i+1]);
      str[i+1]=str[i];
    }
    //    fprintf(stderr,"copy %c-->str[%d](%c)\n",chr,pos,str[pos]);
    str[pos]=chr;
  }

  return str;

}

char* Delete_character(char *str,int pos){

  /* str..一文字削除する文字列 */
  /* pos..削除場所  posは0からstrlen(str)-1まで */

  int i;

  //  fprintf(stderr,"Delete str: %s pos: %d\n",str,pos);

  if (strlen(str)>pos && pos >= 0){
    /* str[pos+1],...,str[strlen(str)]を１つずつ前にずらす  */
    for(i=pos;i<strlen(str);i++) {
      //      fprintf(stderr,"copy str[%d](%c)-->str[%d](%c)\n",i+1,str[i+1],i,str[i]);
      str[i]=str[i+1];
    }
  }

  return str;

}

void swapchar(char *a, char *b){
  char tmp;

  tmp=*a;
  *a=*b;
  *b=tmp;

}
