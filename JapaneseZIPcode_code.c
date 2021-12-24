#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <time.h>

#define ADVANCED 1 //発展課題（絞り込み検索）に対応する場合は1に変更

#define DATAFILE "data_sjis.csv" //data_utf.csvかdata_sjis.csvに変更
#define CLEN 9 //郵便番号の最大バイト長
#define ALEN 200 //住所欄の最大バイト長
#define MAX_SIZE 200000//住所録中の住所数の最大数
long dataset_length;
int mode; //検索モード 0:なし，1:郵便番号検索，2:文字列検索
int refine_flag; //絞り込み検索の有無 0:なし，1:あり
char query[ALEN]; //検索クエリ（郵便番号or文字列）
int label_code[MAX_SIZE];
int label_pref[MAX_SIZE];
int label_city[MAX_SIZE];
int maxmemory,startmemory;
int correct[MAX_SIZE];
int maxcorrect;
typedef struct{
    int code;
    char pref[ALEN+1];
    char city[ALEN+1];
    char town[ALEN+1];
}data;
data dataset[MAX_SIZE];

typedef struct{
    int before;
    int start;
    int end;
    int next;
}mem;
mem memory[MAX_SIZE];

void print(int label){
    int icode = dataset[label].code;
    while(icode<1000000){
        icode*=10;
        printf("0");
    }
    printf("%d:%s%s%s",dataset[label].code,dataset[label].pref,dataset[label].city,dataset[label].town);
    printf("\n");
}
void label_code_initialize(){
    for(int i=0;i<MAX_SIZE;i++)
    {
        label_code[i] = i;
    }
}
void codeorder(start,end){
    if(start>=end) return;
    int middle = (start+end)/2,temp,temp_label;
    int data_middle = dataset[label_code[middle]].code;
    int label_middle = label_code[middle];
    label_code[middle]= -1;
    int left = start;
    int right = end;
    int pin = 0;
    int data_pin;
    int data_pin_label;
    while(label_code[left+pin]==-1){
            pin++;
    }
    data_pin = dataset[label_code[left+pin]].code;
    data_pin_label = label_code[left+pin];
    label_code[left+pin] = -1;
    while(left!=right && data_pin!=-1)
    {
        if(data_pin<=data_middle)
        {
            if(label_code[left]==-1){
                label_code[left] = data_pin_label;
                left++;
                if(right==left) break;
                while(label_code[left+pin]==-1){
                    pin++;
                }
                data_pin = dataset[label_code[left+pin]].code;
                data_pin_label = label_code[left+pin];
                label_code[left+pin] = -1;
            }
            else{
                data_pin = dataset[label_code[left]].code;
                temp_label = data_pin_label;
                data_pin_label = label_code[left];
                label_code[left]= temp_label;
                left++;
            }
        }
        else{
            if(label_code[right]==-1){
                label_code[right]=data_pin_label;
                right--;
                if(right==left) break;
                while(label_code[left+pin]==-1){
                    pin++;
                }
                data_pin = dataset[label_code[left+pin]].code;
                data_pin_label = label_code[left+pin];
                label_code[left+pin] = -1;
            }
            else{
                temp = data_pin;
                data_pin = dataset[label_code[right]].code;
                temp_label = data_pin_label;
                data_pin_label = label_code[right];
                label_code[right]= temp_label;
                right--;
            }
        }
    }
    if(label_code[left]==-1) {
            label_code[left] = label_middle;
            codeorder(start,left-1);
            codeorder(left+1,end);
    }
    else {
            label_code[right] = label_middle;
            codeorder(start,right-1);
            codeorder(right+1,end);
    }
}

int convert_string_to_num(char line[],int start) // Convert string to num
{
    int ans = 0;
    int minus = 0;
    for(int i=start;i<=start+6;i++)
    {
        if(line[i]>='0' && line[i]<='9'){
            ans*=10;
            ans+=(line[i]-'0');
        }
    }
    return ans;
}

int stringcompare(char line1[],char line2[]){
    int i;
    for(i=0;line1[i]!='\0';i++)
    {
        if(line1[i]==line2[i]);
        else return 0;
    }
    if(line1[i]=='\0' && line2[i]=='\0')return 1;
    return 0;
}

int stringrightinleft(char line1[],char line2[]){
    int j=0;
    for(int i=0;line1[i]!='\0';i++)
    {
        if(line2[j]=='\0') return 1;
        if(line1[i]==line2[j]){
            j++;
        }
        else{
            j=0;
        }
    }
    if(line2[j]=='\0') return 1;
    return 0;
}
//住所データファイルを読み取り，配列に保存
void deleted(char line[]) // Erase "
{
    int j;
    for(j=0;line[j+1]!='\0';j++)
    {
            line[j]=line[j+1];
    }
    line[j-1]='\0';
}
void scan(){
  FILE *fp;
  char code[CLEN+1],pref[ALEN+1],city[ALEN+1],town[ALEN+1];
  long line=0;

  //datasizeの計算
  if ((fp = fopen(DATAFILE, "r")) == NULL) {
    fprintf(stderr,"error:cannot read %s\n",DATAFILE);
    exit(-1);
  }
  while(fscanf(fp, "%*[^,],%*[^,],%[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%*s",code,dataset[line].pref,dataset[line].city,dataset[line].town) != EOF ){
    dataset[line].code = convert_string_to_num(code,1);
    deleted(dataset[line].pref);
    deleted(dataset[line].city);
    deleted(dataset[line].town);
    //printf("%ld : %d %s %s %s\n",line,dataset[line].code,dataset[line].pref,dataset[line].city,dataset[line].town);
    line++;
  }
    dataset_length = line;
  printf("%ld行の住所があります\n",line);
  fclose(fp);
}

void preprocess(){
    //preprocess code
    int collect_pref =0,collect_city =0;
    label_code_initialize();
    codeorder(0,dataset_length);
    //preprocess pref
    for(int line=0;line<dataset_length;line++){
    if(line!=0){
        if(stringcompare(dataset[line].pref,dataset[line-1].pref)==0){
            //printf("****%s different %s******\n",dataset[line].pref,dataset[line-1].pref);
            //printf("%d %d\n\n",collect_pref,line);
            label_pref[collect_pref] = line;
            collect_pref = line;
        }
        if(stringcompare(dataset[line].city,dataset[line-1].city)==0){
            //printf("****%s different %s****** from %d to %d\n",dataset[line].city,dataset[line-1].city,collect_city,line-1);
            label_city[collect_city] = line;
            collect_city = line;
        }
    }
    //printf("%ld : %d %s %s %s\n",line,dataset[line].code,dataset[line].pref,dataset[line].city,dataset[line].town);
    }
    label_pref[collect_pref] = dataset_length;
    label_city[collect_city] = dataset_length;
    return;
}

double diff_time(clock_t t1, clock_t t2){
  return (double) (t2-t1)/CLOCKS_PER_SEC;
}

//初期化処理
void init(){
  clock_t t1,t2;

  t1 = clock();
  scan();
  preprocess();
  printf("Done initilization\n");
  t2 = clock();
  printf("\n### %f sec for initialization. ###\n",diff_time(t1,t2));
}

int binarysearchcode(int query)
{
    long left=0,right=dataset_length;
    while(left<=right)
    {
        long middle = (left+right)/2;
        int middlecode = dataset[label_code[middle]].code;
        if(query == middlecode) return middle;
        else if(query > middlecode) left = middle+1;
        else right = middle-1;
    }
    return -1;
}

//郵便番号による住所検索．検索結果を出力．
void code_search(){
    printf("You are searching for : %s\n",query);
    int answer = binarysearchcode(convert_string_to_num(query,0));
    int left_answer = answer;
    int right_answer = answer;
    while(dataset[label_code[left_answer]].code==dataset[label_code[answer]].code) left_answer--;
    while(dataset[label_code[right_answer]].code==dataset[label_code[answer]].code) right_answer++;
    left_answer++;right_answer--;
    //printf("%d from %d to %d\n",answer,left_answer,right_answer);
    for(int i=left_answer;i<=right_answer;i++ )print(label_code[i]);
    return;
}

//文字列による住所検索．検索結果を出力．
void address_search(){
    maxcorrect = 1;
    int prefi=0;
    int current_memory=0,cityi;
    while(prefi!=dataset_length){
        if(stringrightinleft(dataset[prefi].pref,query)){
            for(int j=prefi;j<label_pref[prefi];j++)
            {
                correct[j]=1;
            }
        }
        else{
            int cityi=prefi;
            while(cityi<label_pref[prefi]){
                if(stringrightinleft(dataset[cityi].city,query)){
                    for(int j=cityi;j<label_city[cityi];j++)
                    {
                        correct[j]=1;
                    }
                }
                else{
                    int towni=cityi;
                    while(towni<label_city[cityi]){
                        if(stringrightinleft(dataset[towni].town,query)){
                            correct[towni]=1;
                        }
                        else{
                            correct[towni]=0;
                        }
                        towni++;
                    }
                }
                cityi = label_city[cityi];
            }
        }
        prefi = label_pref[prefi];
    }
    for(int i=0;i<dataset_length;i++)
    {
        //printf("%d = %d\n",label_code[i],correct[label_code[i]]);
        if(correct[label_code[i]]==1) print(label_code[i]);
    }
    return;
}
int min(int a,int b)
{
    if(a>b) return b;
    return a;
}
//絞り込み検索の実施
//文字列による住所検索．検索結果を出力．

void refinement(){
    maxcorrect++;
    int prefi=0;
    int current_memory=0,cityi;
    while(prefi!=dataset_length){
        if(stringrightinleft(dataset[prefi].pref,query)){
            for(int j=prefi;j<label_pref[prefi];j++)
            {
                correct[j]++;
            }
        }
        else{
            int cityi=prefi;
            while(cityi<label_pref[prefi]){
                if(stringrightinleft(dataset[cityi].city,query)){
                    for(int j=cityi;j<label_city[cityi];j++)
                    {
                        correct[j]++;
                    }
                }
                else{
                    int towni=cityi;
                    while(towni<label_city[cityi]){
                        if(stringrightinleft(dataset[towni].town,query)){
                            correct[towni]++;
                        }
                        towni++;
                    }
                }
                cityi = label_city[cityi];
            }
        }
        prefi = label_pref[prefi];
    }
    for(int i=0;i<dataset_length;i++)
    {
        if(correct[label_code[i]]==maxcorrect) print(label_code[i]);
    }
    return;
}

void input(){
  printf("\n"
	 "#########Top Menu#########\n"
	 "# Search by postal code: 1\n"
	 "# Search by address    : 2\n"
	 "# Exit                 : 0\n"
	 "> ");
  scanf("%d", &mode);
  if(mode == 1){
    printf("Postal code > ");
    scanf("%s", query);
  }else if(mode == 2){
    printf("Search String > ");
    scanf("%s", query);
  }
}

//絞り込み検索の有無を確認
void re_input(){
  printf("\n"
	 "# Continue Searching: 1\n"
	 "# Return to Top Menu: 0\n"
	 "> ");
  scanf("%d", &refine_flag);
  if(refine_flag == 1){
    printf("String for Refinement> ");
    scanf("%s", query);
  }
  return;
}

//クエリへの応答
void respond(){
  clock_t t1,t2;
  mode = 1;
  while(1){
    input();
    if(mode == 1){
      t1 = clock();
      code_search();
      t2 = clock();
      printf("\n### %f sec for search. ###\n", diff_time(t1,t2));
    }
    else if(mode == 2){
      t1 = clock();
      address_search();
      t2 = clock();
      printf("\n### %f sec for search. ###\n", diff_time(t1,t2));
      if(!ADVANCED) continue;
      while(1){
	re_input();
	if(refine_flag == 0) break;
	t1 = clock();
	refinement();
	t2 = clock();
	printf("\n### %f sec for search. ###\n", diff_time(t1,t2));
      }
    }
    else break;
  }
}

void main()
{
  init();
  respond();
}
