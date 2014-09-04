/*
  Automatic Enbugging Program

  by Takeru "TODO" Zinbei MIYAZAKI

   start       2014/01/16
   ver. 0.13   2014/01/17 



   ver. 0.10   2014/01/16  固定の置換テーブルによる文字の置き換えエラー実装
   ver. 0.11   2014/01/16  置換テーブルをファイルから読み込む機能を追加
   ver. 0.12   2014/01/16  ==を=と間違えるという基本的なエラーを追加
   ver. 0.13   2014/01/17  {に対応する}が欠落する、と、括弧の種類が違うというエラーを追加
   ver. 0.14   2014/01/17  エンバグする個数の指定と、エンバグ個所の表示On/Offの切り替え,ヘルプ表示追加 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 入力バッファサイズ */
#define BUFSIZE 1024

/* 入力プログラムの最大行数 */
#define MAXLINE 65536


#define REPLACEGROUP 16
#define REPLACELEN 16
#define REPLACETABLEFILENAME "ReplaceTable.txt"

struct RepTable {
  int groupnum;
  char group[REPLACEGROUP][REPLACELEN];
  int charanum[REPLACEGROUP];
};

#define Bracketsnum 4
char Brackets[2][Bracketsnum+1]={"{[(<","}])>"};


void Delete_remark(char**,int);
void Enbug(char**,char **,int,int,struct RepTable *);

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
    //  else {
    //    }

  }
  // else {
  //  }

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
	  if (nrp[i][j]=='%' || nrp[i][j]=='\\'){
	    j++;  // skip it and its following character
	    break;
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
  int count;
  int c,i,j,r,s,k;
  


  for(count=0;count<abs(nob);){

    c=rand()%line;  // Choose random line
    j=rand()%(strlen(nrp[c])); // Choose random character


    // Replace by using Replace-table
    for(r=0;r<rt->groupnum;r++) {

      for(i=0;i<rt->charanum[r];i++) {

	if(nrp[c][j] == rt->group[r][i]) {

	  s = (i + rand()%((rt->charanum[r]) - 1)+1)%(rt->charanum[r]);
	  
	  prog[c][j] = rt->group[r][s];

	  if (nob < 0)
	    fprintf(stderr,"change %c into %c in line %d (%d)\n",nrp[c][j],prog[c][j],c,j);
	  nrp[c][j]=' '; // it has already been replaced
	  count++;
	  r=rt->groupnum;
	  i=rt->charanum[r];
	}
      }

    }

    // Change == into = (one of the most frequently error)
    if(nrp[c][j] == '=') {

      if ((j > 0 && nrp[c][j-1]=='=') || nrp[c][j+1]=='=' ) {

	if (nob < 0)
	  fprintf(stderr,"change == into = in line %d (%d)\n",c,j);

	prog[c][j]=' ';
	nrp[c][j]=' '; 
	count++;
      }

    }

    // Lost } corresponding to {
    if(nrp[c][j] == '}') {
      
      if (nob < 0)
	fprintf(stderr,"delete } in line %d (%d)\n",c,j);
      
      prog[c][j]=' ';
      nrp[c][j]=' '; 
      count++;
      
    }


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
	    fprintf(stderr,"replace %c...%c into %c...%c in line %d (%d...%d)\n",nrp[c][j],nrp[c][r],prog[c][j],prog[c][r],c,j,r);
	  
	  nrp[c][j]=' '; // it has already been replaced
	  nrp[c][r]=' '; 
	  count++;
	  break;
	}
      }
    }




  }



}
