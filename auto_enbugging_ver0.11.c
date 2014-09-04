/*
  Automatic Enbugging Program

  by Takeru "TODO" Zinbei MIYAZAKI

   ver. 0.12   2014/01/16 

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

struct RepTable {
  int groupnum;
  char group[REPLACEGROUP][REPLACELEN];
  int charanum[REPLACEGROUP];
};


void Delete_remark(char**,int);
void Enbug(char**,char **,int,int,struct RepTable *);

int main(int argc, char *argv[]){

  FILE *input_fp, *output_fp;
  char buf[BUFSIZE];   /* 読み込みバッファ */
  char **prog;         /* プログラム格納用文字列の配列ポインタ */
  char **noremprog;    /* 注釈文を省いたプログラムの格納用ポインタ */
  int line;            /* 読み込んだプログラムの行数 */
  int i;
  struct RepTable r_table;

  // init. rand()
  srand( (unsigned int)time(0));


  // Replace Table Init.
  r_table.groupnum = 2;
  strcpy(r_table.group[0],"ijl1");
  r_table.charanum[0]=strlen(r_table.group[0]);
  strcpy(r_table.group[1],"nm");
  r_table.charanum[1]=strlen(r_table.group[1]);

  // File Open
  if (argc > 1) {

    /* 入力ファイルを開く */
    if ((input_fp=fopen(argv[1],"r"))==NULL) {
      fprintf(stderr, "ERROR! Cannot open file : %s\n",argv[1]);
      exit(1);
    }
    
    if (argc > 2) {
      
      /* 出力ファイルを開く */
      if ((output_fp=fopen(argv[2],"w"))==NULL) {
	fprintf(stderr, "ERROR! Cannot open file : %s\n",argv[2]);
	exit(1);
      }
      
    } else {

    /* エンバグしたプログラムを標準出力へ出力する  */
    output_fp=stdout;

    }

  } else {

    /* 変換する元のプログラムを標準入力から入力する  */
    input_fp=stdin;

  }

  // Read the Original Program

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

  /* output no-remark program

  for(i=0;i<line;i++){
    fprintf(stderr,"%s",noremprog[i]);
  }
   */  


  // Enbugging!
  Enbug(prog,noremprog,line,3,&r_table);


  // Output Enbagged program
  for(i=0;i<line;i++) {
    fputs(prog[i],output_fp);
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

  int flag=0;
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
    if (flag==1) {
      flag=0;
    }
  }

}


void Enbug(char **prog,char **nrp,int line,int nob,struct RepTable *rt){
  int count;
  int c,i,j,r,s;
  


  for(count=0;count<nob;){

    c=rand()%line;  // Choose random line
    j=rand()%(strlen(nrp[c])); // Choose random character

    for(r=0;r<rt->groupnum;r++) {

      for(i=0;i<rt->charanum[r];i++) {

	if(nrp[c][j] == rt->group[r][i]) {

	  s = (i + rand()%((rt->charanum[r]) - 1)+1)%(rt->charanum[r]);
	  
	  prog[c][j] = rt->group[r][s];

	  fprintf(stderr,"change %c into %c in line %d (%d)\n",nrp[c][j],prog[c][j],c,j);
	  count++;
	  r=rt->groupnum;
	  i=rt->charanum[r];
	}
      }

    }

    if(nrp[c][j] == '=') {

      if ((j > 0 && nrp[c][j-1]=='=') || nrp[c][j+1]=='=' ) {

	fprintf(stderr,"change == into = in line %d (%d)\n",c,j);

	prog[c][j]=' ';
	count++;
      }

    }

  }



}
