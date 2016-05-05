/*
 * Programing Language C-- "Compiler"
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2014-2016 by
 *                      Dept. of Computer Science and Electronic Engineering,
 *                      Tokuyama College of Technology, JAPAN
 *
 *   上記著作権者は，Free Software Foundation によって公開されている GNU 一般公
 * 衆利用許諾契約書バージョン２に記述されている条件を満たす場合に限り，本ソース
 * コード(本ソースコードを改変したものを含む．以下同様)を使用・複製・改変・再配
 * 布することを無償で許諾する．
 *
 *   本ソースコードは＊全くの無保証＊で提供されるものである。上記著作権者および
 * 関連機関・個人は本ソースコードに関して，その適用可能性も含めて，いかなる保証
 * も行わない．また，本ソースコードの利用により直接的または間接的に生じたいかな
 * る損害に関しても，その責任を負わない．
 *
 *
 */

/*
 * syntax.c : C--トランスレータのコード生成部
 *
 * 2016.05.04 v3.0.0  : デバッグ完了バージョン(重村)
 * 2014.04.29 v2.1.0  : 初期バージョン(柳くん)
 *
 */

#include <stdio.h>
#include "util.h"
#include "code.h"
#include "namtbl.h"
#include "sytree.h"

static void traceTree(int node);                    // 再帰呼び出しのため
static boolean inhibitOut = false;                  // 出力を抑制する

// 構造体や配列の初期化用仮変数名（ラベル）を出力する
static void printTmpLab(char *name, int num) {
  printf("_cmm_%s_%d", name, num);                  // "_cmm_名前_番号"
}

// 文字列用の仮変数名（ラベル）を出力する
static void printStrLab(int lab) {
  printf("_cmm_%dS", lab);                          // "_cmm_番号S"
}

// ローカル変数名を出力する
static void printLocVar(int num) {
  printf("_cmm_%dL", num);                          // "_cmm_番号L"
}

// 仮引数名を出力する
static void printParam(int num) {
  printf("_cmm_%dP", num);                          // "_cmm_番号P"
}

// 型を出力する
static void printTypeName(int typ) {
  if (typ==TyVOID) printf("void ");                 // "void "
  else if (typ==TyINT) printf("int ");              // "int "
  else if (typ==TyBOOL) printf("char ");            // boolean なら "char "
  else if (typ==TyCHAR) printf("char ");            // "char "
  else if (typ==TyINTR) error("interrupt使用禁止"); // エラー
  else if (typ==TyDOTDOTDOT) printf("...");         // "..."
  else if (typ <= 0 && ntGetType(-typ) == TySTRUC)  // 構造体なら
      printf("struct %s ",ntGetName(-typ));         //   "struct 構造体名 "
  else if (typ <= 0 && ntGetType(-typ) == TyREF)    // typedef なら
      printf("%s ",ntGetName(-typ));                //   "型名 "
  else error("バグ...printTypeName");               // 型ではなかったとき
}

// 型を出力する
static void printType(boolean sta, int typ, int nAst) {
  if (typ <= 0) nAst = nAst + 1;                    // 参照型なら * を追加

  if (sta) printf("static ");                       // "[static ]"
  printTypeName(typ);                               // "型名"
  for (int i=0; i<nAst; i=i+1)                      // "[*...]"
    printf("*");                                    // 
}

static void printTypeIdx(int idx) {
  boolean sta = !ntGetPub(idx);                     // static
  int typ = ntGetType(idx);                         // 型
  int dim = ntGetDim(idx);                          // 次元
  printType(sta, typ, dim);                         // "[static] 型名[*...]"
}

// ローカル変数の宣言を出力
static void printLocDecl(int idx) {
  int typ = ntGetType(idx);
  int dim = ntGetDim(idx);
  printType(false, typ, dim);                       // "型名[*...]"
  printf("%s", ntGetName(idx));                     // "名前"
}

// グローバル変数、関数の宣言を出力
static void printGlobDecl(int idx) {
  printTypeIdx(idx);                                // "[static] 型名[*...]"
  printf("%s", ntGetName(idx));                     // "名前"
}

// 後置演算以外の演算子の出力
static void printOP(int op){
  if (op==SyNEG)       printf("-");                 // 単項演算子 -
  else if (op==SyNOT)  printf("!");                 // 単項演算子 !
  else if (op==SyBNOT) printf("~");                 // 単項演算子 ~
  else if (op==SyADD)  printf("+");                 // ２項演算子 +
  else if(op==SySUB)   printf("-");                 // ２項演算子 -
  else if(op==SySHL)   printf("<<");                // ２項演算子 <<
  else if(op==SySHR)   printf(">>");                // ２項演算子 >>
  else if(op==SyBAND)  printf("&");                 // ２項演算子 &
  else if(op==SyBXOR)  printf("^");                 // ２項演算子 ^
  else if(op==SyBOR)   printf("|");                 // ２項演算子 |
  else if(op==SyMUL)   printf("*");                 // ２項演算子 *
  else if(op==SyDIV)   printf("/");                 // ２項演算子 /
  else if(op==SyMOD)   printf("%%");                // ２項演算子 %
  else if(op==SyGT)    printf(">");                 // ２項演算子 >
  else if(op==SyGE)    printf(">=");                // ２項演算子 >=
  else if(op==SyLT)    printf("<");                 // ２項演算子 <
  else if(op==SyLE)    printf("<=");                // ２項演算子 <=
  else if(op==SyEQU)   printf("==");                // ２項演算子 ==
  else if(op==SyNEQ)   printf("!=");                // ２項演算子 !=
  else if(op==SyASS)   printf("=");                 // ２項演算子 =
  else if(op==SyCOMM)  printf(",");                 // ２項演算子 ,
  else if(op==SyOR)    printf("||");                // ２項演算子 ||
  else if(op==SyAND)   printf("&&");                // ２項演算子 &&
  else error("バグ...printOP");                     // 演算子でなかった
}

// 再帰のために宣言が必要
static void printExp(int node);

// 関数呼び出し時に実引数を出力
static void printArgs(int node){
  if(node==SyNULL) return;
  if(syGetType(node)==SySEMI) {                     // 引数リストなら
    printArgs(syGetLVal(node));                     //   左ノードを出力
    printf(",");                                    //   引数の間に , を出力
    printArgs(syGetRVal(node));                     //   右ノードを出力
  } else {                                          // 引数なら
    printExp(node);                                 //   引数式を出力
  }
}

// 式の出力
static void printExp(int node){
  int typ = syGetType(node);
  int lVal = syGetLVal(node);
  int rVal = syGetRVal(node);

  if(typ==SyCNST) {                                 // 定数なら
    printf("%d",syGetLVal(node));                   //   "値"
  } else if(typ==SySTR) {                           // 文字列なら
    printStrLab(lVal);                              //   "文字列名"
  } else if (typ==SyLOC) {                          // ローカル変数なら
    printLocVar(lVal);                              //   "ローカル変数名"
  } else if (typ==SyPRM) {                          // 仮引数なら
    printParam(lVal);                               //   "仮引数名"
  } else if (typ==SyGLB) {                          // グローバル変数なら
    printf("%s",ntGetName(lVal));                   //   "変数名"
  } else if (typ==SyLABL) {                         // addrofなら
    error("addrof は使用できない");                 //   エラー
  } else if (typ==SyFUNC) {                         // 関数なら
    printf("%s(", ntGetName(lVal));                 //   "関数名([実引数,...])"
    printArgs(rVal);                                //
    printf(")");                                    //
  } else if (typ==SyIDXW || typ==SyIDXB) {          // [] なら
    printExp(lVal);                                 //   "左辺式[右辺式]"
    printf("[");                                    //
    printExp(rVal);                                 //
    printf("]");                                    //
  } else if(typ==SyDOT) {                           // . なら
    printExp(lVal);                                 //   "左辺式->フィールド名"
    printf("->");                                   //
    printf("%s", ntGetName(syGetLVal(rVal)));       //
  } else if(typ==SyCHAR) {                          // chr() なら
    printf("(0xff&");                               //   念のため0xffでマスク
    printExp(lVal);                                 //   "(0xff & 式)"
    printf(")");                                    //
  } else if(typ==SyBOOL) {                          // bool() なら
    printf("(0x01&");                               //   念のため1でマスク
    printExp(lVal);                                 //   "(0x01 & 式)"
    printf(")");                                    //
  } else if (SyIS1OPR(typ)) {                       // 後置以外の単項演算なら
    printf("(");                                    //   "(演算子 式)"
    printOP(typ);                                   //
    printExp(lVal);                                 //
    printf(")");                                    //
  } else {                                          // それ以外なら２項演算
    printf("(");                                    //   "(左辺 演算子 右辺)"
    printExp(lVal);                                 //
    printOP(typ);                                   //
    printExp(rVal);                                 //
    printf(")");                                    //
  }
}

// if文の出力
static void printIf(int node){
  printf("if (");                                   // if(
  printExp(syGetLVal(node));                        // 条件式
  printf(")");                                      // )
  traceTree(syGetRVal(node));                       // 文
}

// if-else文の出力
static void printEls(int node){
  printIf(syGetLVal(node));                         // if文
  printf("else ");                                  // else
  traceTree(syGetRVal(node));                       // 文
}

// while文の出力
static void printWhl(int node){
  int lVal = syGetLVal(node);                       // 条件式
  int rVal = syGetRVal(node);                       // 本体

  printf("while(");                                 // "while("
  if(lVal==SyNULL)                                  // 条件式がない場合は
    printf("1");                                    //   "1" (無限ループ)
  else                                              // 条件式があれば
    printExp(lVal);                                 //   "条件式"
  printf(")");                                      // ")"

  printf("{\n");
  traceTree(rVal);                                  // "{ 本体; 再初期化式 }"
  printf("}\n");
}

// do-while文の出力
static void printDo(int node){
  int lVal = syGetLVal(node);                       // 本体
  int rVal = syGetRVal(node);                       // 条件式

  printf("do");                                     // "do"
  traceTree(lVal);                                  // "本体"
  printf("while(");                                 // "while ("
  printExp(rVal);                                   // "条件式"
  printf(");\n");                                   // ");"
}

// break文の出力
static void printBrk(int node){
  printf("break;\n");                               // "break;"
}

// continue文の出力
static void printCnt(int node){
  printf("continue;\n");                            // "continue;"
}

// return文の出力
static void printRet(int node){
  int lVal = syGetLVal(node);                       // 式

  printf("return ");                                // "return "
  if (lVal!=SyNULL) printExp(lVal);                 // 式があれば "式"
  printf(";\n");                                    // ";"
}

// ローカル変数宣言の出力(変数名は番号から作る)
static void printVAR(int node){  
  int rVal = syGetRVal(node);
  int typ  = syGetLVal(rVal);                       // 型
  int dim  = syGetRVal(rVal);                       // 次元
  printType(true, typ, dim);                        // "型名[*...]"
  printLocVar(syGetLVal(node));                     // "ローカル変数名"
  printf(";\n");                                    // ";"
}

// ブロックの出力
static void printBLK(int node){
  printf("{\n");                                    // "{"
  traceTree(syGetLVal(node));                       // 先に左側をコード生成
  traceTree(syGetRVal(node));                       // 次に右側をコード生成
  printf("}\n");                                    // "}"
}

static void traceTree(int node){
  if (node==SyNULL) return;                         // 何も無い
  int typ = syGetType(node);
  if (typ==SyIF) printIf(node);                     // if 文
  else if (typ==SyELS) printEls(node);              // if-else 文
  else if (typ==SyWHL) printWhl(node);              // while 文
  else if (typ==SyDO)  printDo(node);               // do-while 文
  else if (typ==SyBRK) printBrk(node);              // break 文
  else if (typ==SyCNT) printCnt(node);              // continue 文
  else if (typ==SyRET) printRet(node);              // return 文
  else if (typ==SyVAR) printVAR(node);              // ローカル変数宣言
  else if (typ==SyBLK) printBLK(node);              // ブロック
  else if (typ==SySEMI) {                           // セミコロン
    traceTree(syGetLVal(node));                     // 先に左側をコード生成
    traceTree(syGetRVal(node));                     // 次に右側をコード生成
  } else {                                          // 式文
    printExp(node);                                 // "式;"
    printf(";\n");
  }
}    

// 関数宣言を出力
static void printFuncDecl(int idx) {
  printGlobDecl(idx);                               // "[static]型名[*...]名前"
  printf("(");                                      // "("
  int cnt = ntGetCnt(idx);                          // 仮引数の数
  for (int i=1; i<=cnt; i=i+1) {
    int typ = ntGetType(idx+i);                     // 型
    int dim = ntGetDim(idx+i);                      // 次元
    
    printType(false, typ, dim);                     // "型名[*...]"
    printParam(i);                                  // "仮引数名”
    if (i<cnt) printf(",");                         // ","
  }
  printf(")");                                      // ")"
}

// プロトタイプ宣言の出力
void genProto(int idx) {
  if (inhibitOut) return;

  printFuncDecl(idx);                               // "[
  printf(";\n");                                    // ";"
}

// 関数の出力
void genFunc(int funcIdx,boolean prot, boolean kernFlg){
  if (inhibitOut) return;

  printFuncDecl(funcIdx);                           // "関数宣言""
  int root = syGetRoot();
  printf("{\n");                                    // "{"
  if (root!=SyNULL && syGetType(root)==SyBLK)       // 必要以上に"{ }"を
    sySetType(root, SySEMI);                        //   出力しないように
  traceTree(root);                                  // "関数本体"
  printf("}\n");                                    // "}"
}

// 配列のラベルの割当
static int curLab = 0;
static int newLab(){
  int l = curLab;
  curLab = curLab + 1;
  return l;
}

// 配列や構造体の初期化で使用する一時変数の宣言を出力
static void printTmpValDcl(int vType, int nAst, char *name) {
  printType(true, vType, nAst);                        // "static 型名[*...]"
  printTmpLab(name, newLab());                         // "_cmm_%s_%d"
}

// 中間のポインタ配列を出力
static void printPtrArray(int vType, int nAst, char *name, int cnt, int lab) {
  printTmpValDcl(vType, nAst, name);                   // "static 型名[*...]
  printf("[]={");                                      //  _cmm_%s_%d[]={"
  printTmpLab(name, lab);                              // "_cmm_%s_%d"
  for (int j=1; j<cnt; j=j+1) {                        // 要素数の数だけ
    printf(",");                                       //  ",_cmm_%s_%d"  
    printTmpLab(name, lab+j);                          //
  }
  printf("};\n");                                      // "};"
}

// printArray0 : "array(n1, n2, ... )" に対応する部分を出力
//   帰値  : この次元のインスタンスの仮編変数名の開始番号
//   vType : 初期化している変数の型
//   nAst  : いま着目している次元は何重の間接ポインタになるか
//   name  : 初期化している変数の名前
//   node  : この次元の要素数を示すSyCNSTか、SyCNSTが右にあるSySEMI
//   cnt   : この次元で必要なインスタンスの数
static int printArray0(int vType, int nAst, char* name, int node, int cnt) {
  int typ  = syGetType(node);
  int rVal = syGetRVal(node);
  int lVal = syGetLVal(node);
  int l = curLab;

  if (typ==SySEMI) {                                   // 最後の次元でない時
    if (syGetType(rVal)!=SyCNST)                       // 定数でなければエラー
      error("バグ...genArray0_1");

    int size = syGetLVal(rVal);                        // 次の次元の要素数

    // 次の次元の配列を生成
    int ln = printArray0(vType, nAst-1, name, lVal, cnt*size);

    l = curLab;
    for (int i=0; i<cnt; i++) {                        // 前の次元の要素数分
      printPtrArray(vType, nAst, name, size, ln);      // 中間のポインタ配列
      ln = ln + size;
    }
  } else if (typ==SyCNST) {                            // 最後の次元の時
    for (int i=0; i<cnt; i++) {                        // 前の次元の要素数分
      printTmpValDcl(vType, nAst, name);               // "static 型名[*...]
      printf("[%d];\n",syGetLVal(node));               //  _cmm_%s_%d[%d];"
    }
  } else error("バグ...genArray0_2");                  // それ以外ならバグ
  return l;
}

// 非初期化配列の出力
static void printArray(int node, int idx){
  int lVal = syLVal(node);                             // SyARRAY の左辺
  int typ = ntGetType(idx);                            // 配列のデータ型
  int nAst = ntGetDim(idx) - 1;                        // インスタンスの * の数
  char *name = ntGetName(idx);                         // 配列変数名

  int ln = printArray0(typ, nAst, name, lVal, 1);      // 配列インスタンス出力
  printGlobDecl(idx);                                  // "[static] 型名[*...]
  printf("=");                                         //  配列変数名="
  printTmpLab(name, ln);                               // "_cmm_%s_%d;
  printf(";\n");
}

// 初期化配列の出力
static void printList1(int node, int idx, int n, int dim){
  int typ  = syGetType(node);
  int lval = syGetLVal(node);
  int rval = syGetRVal(node);
  if(n == 1)                                        // 二個目以降なら
    printf(",");                                    //  ','もいる
  if (typ==SyCNST)                                  // 数なら
    printf("%d",lval);                              //  ただ出力
  else if(typ==SySTR)                               // 文字列なら
    printStrLab(lval);                              //  文字列変数名を出力    
  else if(typ==SyLIST){                             // リストなら
    if(ntGetType(idx)<=0&&dim==2)                   //  構造体だったなら
      printf("&");                                  //   '&'が必要
    printTmpLab(ntGetName(idx), rval);              //  仮変数を出力
  } else if(typ==SyARRY) {                          // 非初期化配列だったなら
    printTmpLab(ntGetName(idx), rval);              // 右ノードに添え字がある
  }
} 

// 配列の初期化コード生成
static void printList0(int node, int idx, int dim, int dim2, int n) {
  int typ  = syGetType(node);
  int lVal = syGetLVal(node);
  int rVal = syGetRVal(node);
  if (typ==SySEMI) {                                // リストの途中
    printList0(lVal, idx, dim, dim2, 0);            //  左から
    printList0(rVal, idx, dim, dim2, 1);            //  右の順番で辿る
  } else if (dim==0) {                              // 出力すべき深さ      
    printList1(node,idx,n,dim2);                    //  データを出力  
  } else if (typ==SyLIST) {                         // 初期化配列を発見
    if (dim==1) {                                   // コード生成直前
      int l = newLab();                             //  ラベル番号を取り出し
      sySetRVal(node, l);                           //  番号を保存
      printf("static ");                            //  必ず'static'を出力
      printTypeIdx(idx);                            //  型を出力
      printTmpLab(ntGetName(idx), l);               // 変数名の出力
      if(!(ntGetType(idx)<=0&&dim==dim2))
	printf("[]");                               // '[]'を出力
      printf(" = {");
      printList0(lVal, idx, dim-1, dim2, n);        // 配列の本体を辿る
      printf("};\n");
    } else
      printList0(lVal, idx, dim-1, dim2-1, 0);      // 配列の本体を辿る
  } else if (typ==SyARRY) {                         // 非初期化配列を含む
    error("バグ...printList0");
    //if (dim==1){                                    // 出力するべき深さなら
    //  int l = printArray0(lVal, 1, idx);            // 非初期化配列生成
    //  sySetRVal(node, l);                           // ラベル番号を保存する
   // }
  }
}

// 初期化リスト({e1, e2, ...})の出力
static void printList(int node, int idx) {
  char *name = ntGetName(idx);                      // 変数名

  int ln = printList0(node, idx, d, dim, 0);        // リストを出力する
  printTypeIdx(idx);                                // "[static] 型名[*...]"
  printf("%s=", name);                              // "変数名="
  printTmpLab(name, ln);                            // "_cmm_%s_%d;"
  printf(";\n");
}

// 初期化データの出力
void genData(int idx){
  curLab = 0;                                       // 配列用ラベル番号初期化
  if (inhibitOut) return;

  int root = syGetRoot();                           // 構文木の先頭取得
  if (root==SyNULL) error("バグ...genData");        // 構文木が必ず必要

  int typ  = syGetType(root);                       // ノードの種類を取得
  if(typ==SyARRY)                                   // v=array(n1,n2,...)の場合
    printArray(root, idx);                          //  非初期化配列の出力
  else if (typ==SyLIST)                             // v = {e1,e2,...} の場合
    printList(root, idx);                           //  初期化記述の出力
  else {                                            // v = 式 の場合
    printTypeIdx(idx);                              // "[static] 型名[*...]"
    printf("%s=", ntGetName(idx));                  // "変数名="
    printExp(root);                                 // "式;"
    printf(";\n");                                  //
  }
}

// 非初期化変数の出力
void genBss(int idx){
  if (inhibitOut) return;

  printTypeIdx(idx);                                // 型を出力
  printf("%s;\n", ntGetName(idx));                  // 名前を出力
}

// 文字列のラベルの割当
static int slab = 0;
static int newSLab(){
  int l = slab;
  slab++;
  return l;
}

// 文字列の出力
int genStr(char *str){
  if (inhibitOut) return 0;

  int lab = newSLab();                              // ラベルを割り付ける
  printf("#define ");
  printStrLab(lab);                                 // ラベルを印刷
  printf(" \"%s\"\n", str);                         // 文字列を出力
  return lab;                                       // ラベル番号を返す
}

// 構造体宣言の出力
void genStruc(int idx){
  if (inhibitOut) return;

  int cnt = ntGetCnt(idx);                          // 構造体の要素数を取得
  printf("struct %s {\n",ntGetName(idx));           // 構造体名を出力
  for(int i=1; i<=cnt; i++){                        // 要素数だけ
    printf("  ");
    printTypeIdx(idx+i);                            // 型を出力
    printf("%s;\n",ntGetName(idx+i));               // 名前を出力
  }
   printf("};\n");  
}

// 出力許可
void genOn() {
  inhibitOut = false;
}

// インクルードディレクティブを印刷して以降の出力禁止
void genOff(char *hdr){
  printf("#include <%s>\n", hdr);
  inhibitOut = true;
}
