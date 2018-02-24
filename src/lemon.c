/*!
 * \file lemon.c
 * \brief lemon源码学习
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h> // 包含各种类型判断函数,比如isdigit()
#include <assert.h>

#define ISSPACE(x) isspace((unsigned char)(x)) ///< 判断是否为空白符
#define ISDIGIT(x) isdigit((unsigned char)(x)) ///< 判断是否为数字
#define ISALNUM(x) isalnum((unsigned char)(x)) ///< 判断是否为字母或数字
#define ISALPHA(x) isalpha((unsigned char)(x)) ///< 判断是否为字母
#define ISUPPER(x) isupper((unsigned char)(x)) ///< 判断是否为大写字母
#define ISLOWER(x) islower((unsigned char)(x)) ///< 判断是否为小写字母

// 跨平台条件编译
#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32)
#        define __WIN32__
#   endif
#endif

#ifdef __WIN32__
#ifdef __cplusplus
extern "C"{
#endif
extern int access(const char* path,int mode);
#ifdef __cplusplus
}
#endif
#else
#include <unistd.h> // Unix操作系统标准API头文件
#endif

#define lemonStrlen(x) ((int)(strlen(x))) ///<计算char*字符串长度

/**
 * @brief 把src的字符串拷贝到dest字符串里
 * @param dest 目标字符串
 * @param src 源字符串
 */
static void lemon_strcpy(char *dest,const char* src){
    while ((*(dest++)=*(src++))!=0){}
}
/**
 * @brief 把src字符串拼接到dest字符串末尾
 * @param dest 目标字符串
 * @param src 源字符串
 */
static void lemon_strcat(char* dest,const char* src){
    while(*dest) dest++; // 把dest指针移动到末尾的'\0'地址
    lemon_strcpy(dest,src);
}

/* lemon重要共享结构前向声明 */
struct rule;
struct lemon;
struct action;


static int showPrecedenceConflict = 0; ///< ???
static char* msort(char*,char**,int(*)(const char*,const char*));///< ???


/* Option 相关声明 */
/// \brief 选项类型(枚举从1开始)
enum option_type {
    OPT_FLAG=1,
    OPT_INT,
    OPT_DBL,
    OPT_STR,
    OPT_FFLAG,
    OPT_FINT,
    OPT_FDBL,
    OPT_FSTR
};
/// \brief 选项结构体
struct s_options{
    enum option_type type; ///< 选项类型
    const char * label;    ///< 选项标签,比如 lemon-b 中的"b"
    char* arg;             ///< 选项附加的参数值(地址)
    const char* message;   ///< 选项信息(介绍选项功能)
};
int OptInit(char**,struct s_options*,FILE*);
int OptNArgs(void);
char* OptArg(int);
void OptErr(int);
void OptPrint(void);

/* lemon 共享struct */
/// \brief lemon布尔常量枚举
typedef enum {LEMON_FALSE=0,LEMON_TRUE} Boolean;

/// \brief symbol类型枚举
enum symbol_type{
    TERMINAL,     ///< 终结符
    NONTERMINAL,  ///< 非终结符
    MULTITERMINAL ///< multi terminal
};
/// \brief 描述结合性的枚举
enum e_assoc{
    LEFT,  ///< 左结合
    RIGHT, ///< 右结合
    NONE,  ///< 无结合性
    UNK    ///< 未知结合性
};

/// \brief symbol(符号)结构体
struct symbol{
    const char *name; ///< 符号名称
    int index;        ///< 符号的索引
    enum symbol_type type; ///< 符号类型,一般就两种:terminal(T)和nonterminal(NT)
    struct rule* rule;///< 当前符号的文法规则链表(只有符号是非终结符才有这个链表)
    struct symbol* fallback; ///< 不需要进行语法分析的符号指针(在语法文件以%fallback标识)
    int prec;           ///< 符号优先级(如果符号没有定义优先级就令prec=-1)
    enum e_assoc assoc; ///< 符号的结合性(注意声明符号的结合性之前必须定义符号的优先级)
    char*firstset;      ///< 在所有文法规则里与这个符号相关的first集
    Boolean lambda;     ///< 如果lambda=true代表非终结符右边的rule可以是一个空字符串(只有非终结符有这个性质)
    int useCnt;         ///< Number of times used
    char* destructor;   ///< 语法文件中用%destructor{}指定的C语言代码,当符号从栈pop时执行这些代码销毁符号
    int destLineno;     ///< 语法文件里%destructor所在行号,如果是重复的destructor就设为-1
    char* datatype;     ///< 当该符号是非终极符(type==NONTERMINAL)时,就需要指定它的数据类型(在语法文件里以%type声明)
    int dtnum;          ///< 与该非终极符相关的数据类型的代号,因为在语法分析器里,所有的数据类型储存在一个union结构体里,只有通过dtnum才能获得正确的数据类型.

    /*  下面两个成员只在符号类型是MULTITERMINAL时使用 */
    int nsubsym;            ///< Number of constituent symbols in the MULTI
    struct symbol **subsym; ///< Array of constituent symbols
};

/// \brief rule(文法规则)结构体
struct rule{
    struct symbol *lhs;
    const char *lhsalias;
    int lhsStart;
    int ruleline;
    int nrhs;
    struct symbol **rhs;
    const char **rhsalias;
    int line;
    const char *code;
    const char *codePrefix;
    const char *codeSuffix;
    int noCode;
    int codeEmitted;
    struct symbol *precsym;
    int index;

};

/// \brief
enum cfgstatus{
    COMPLETE,
    INCOMPLETE
};
/// \brief config结构体,config是对rule进行处理后的结果
struct config{
    struct rule * rp; ///< rp代表config的前身,config是由这个rp指针指向的rule处理得到的
    int dot; ///< 语法分析中界点,数值为已经进栈的符号数量,也就代表了当前正在处理的符号位置
    char *fws;///< 当前config拥有的fellow集(fellow集存放当前config可以利用的所有终结符)
    struct plink *fplp;    ///< fellow集顺向传播链表
    struct plink *bplp;    ///< fellow集逆向传播链表
    struct state *stp;     ///< 包含当前config的状态指针
    enum cfgstatus status; ///< 指示fellow集的处理过程是否结束(两种情况:INCOMPLETE/COMPLETE),另外也作为计算移进(shift)的指示标志
    struct config *next;   ///< 在当前状态中所有config链表的下一个config (next configuration)
    struct config *bp;     ///< 在当前状态中基本config链表的下一个config (next basis configuration)
};

/// \brief
enum e_action{
    SHIFT,
    ACCEPT,
    REDUCE,
    ERROR,
    SSCONFLICT,
    SRCONFLICT,
    RRCONFLICT,
    SH_RESOLVED,
    RD_RESOLVED,
    NOT_USED,
    SHIFTREDUCE
};

/// \brief action结构体
struct action{

};

/// \brief state结构体
struct state{
    struct config *bp;  ///< 当前状态下的基本config组成的链表
    struct config *cfp; ///< 当前状态所有config组成的链表
    int statenum;       ///< 当前状态的代号(顺序索引号)
    struct action *ap;  ///< 当前状态的动作链表
    int nTknAct;        ///< 与动作有关的终结符数量
    int nNtAct;         ///< 与动作有关的非终结符数量
    int iTknOfst;       ///< yy_action[]里储存的终结符偏移量
    int iNtOfst;        ///< yy_action[]里储存的非终结符偏移量
    int iDfltReduce;    ///< 由文法规则规定的默认归约(REDUCE)动作
    struct rule*pDfltReduce;///< 默认归约(REDUCE)动作的文法规则
    int autoReduce;         ///< 如果autoReduce=true,则当前状态是一个自动归约状态(auto-reduce state)
};
#define NO_OFFSET (-2147483647) ///< -2147483647=-2^31,相当于有符号int类型(-2^31 ~ 2^31-1)的最小整数值,是一个边界值

/// \brief plink结构体
struct plink{
    struct config*cfp;
    struct plink*next;
};

/// \brief lemon结构体: 整个语法分析器最核心的结构!
struct lemon{
    struct state ** sorted;  ///< 已排序的状态表
    struct rule* rule;       ///< 储存文法规则的链表
    struct rule* startRule;  ///< 第一条文法规则
    int nstate;              ///< 状态数量
    int nxstate;             ///< 移除tail degenerate(退化/析构)状态后的状态数量
    int nrule;               ///< 文法规则数量
    int nsymbol;             ///< 包括终结符(terminal)和非终结符(nonterminal)的符号数量
    int nterminal;           ///< 终结符数量
    int minShiftReduce;      ///< shift-reduce动作的最小值
    int errAction;           ///< 错误动作(error action)值
    int accAction;           ///< 接受动作(accept action)值
    int noAction;            ///< No-op动作值
    int minReduce;           ///< reduce动作最小值
    int maxAction;           ///< 任意种类动作的最大值
    struct symbol ** symbols;///< 排序后的symbol(符号)指针数组
    int errorcnt;            ///< 出错数量
    struct symbol *errsym;   ///< 错误符号指针
    struct symbol *wildcard; ///< 匹配任何文本的Token符号(记号)
    char* name;              ///< 指定语法生成器(generated parser)的名称,一般是"Parse".
    char* arg;               ///< ???描述parser()的第三个参数
    char* tokentype;         ///< 在解析器栈(parser stack)里终结符(terminal symbols)的类型
    char* vartype;           ///< 非终结符(nonterminal symbols)的默认类型
    char* start;             ///< 语法文件(Grammar)开始解析的第一个符号名称
    char* stacksize;         ///< 解析器栈大小
    char* include;           ///< 在语法文件开头包含的C头文件代码
    char* error;             ///< 语法分析出现错误时的处理代码
    char* overflow;          ///< 语法分析出现栈溢出时的处理代码
    char* failure;           ///< 解析器分析失败时的处理代码
    char* accept;            ///< 解析器分析成功时的处理代码
    char* extracode;         ///< 附加于生成文件末尾的代码
    char* tokendest;         ///< 摧毁终结符数据的代码(终结符析构器)
    char* vardest;           ///< 默认用于摧毁非终结符数据的代码
    char* filename;          ///< 输入的语法文件名称(一般是.y后缀)
    char* outname;           ///< 输出文件名称(默认是C语言文件,即.c后缀,如果有适合的模板文件(lemon自带的是lempar.c),也可以生成java或C++文件
    char* tokenprefix;       ///< 在生成的.h文件里给每一个记号(token)加上前缀tokenprefix,一般前缀都是几个大写字母
    int nconflict;           ///< 语法分析中发生冲突的数量
    int nactiontab;          ///< yy_action[]表中实体(entries)的数量
    int nlookaheadtab;       ///< yy_lookahead[]表中实体的数量
    int tablesize;           ///< 所有表的字节长度总和
    int basisflag;           ///< 在命令行输入lemon -b时就会设置这个basisflag标志,代表只打印输出核心内容而不是全部内容
    int has_fallback;        ///< 如果在语法文件有符号指明是%fallback,那么has_fallback就为true.%fallback标注的符号是尚未投入使用的特殊符号,暂时以普通符号使用
    int nolinenosflag;       ///< 如果nolinenosflag=true,则不打印#line相关的语句 ???
    char* argv0;             ///< 程序名称,就是"lemon"了
};


#define MemoryCheck(x) if((x)==0){ \
    extern void memory_error(); \
    memory_error(); \
} ///< 内存申请错误检查

/* 处理字符串的函数 */
const char* Strsafe(const char*);
void Strsafe_init(void);
int Strsafe_insert(const char*);
const char* Strsafe_find(const char*);

/* 处理语法文件符号的函数 */
struct symbol* Symbol_new(const char*);
int Symbolcmpp(const void*,const void*);
void Symbol_init(void);
int Symbol_insert(struct symbol*,const char*);
struct symbol* Symbol_find(const char*);
struct symbol* Symbol_Nth(int);
int Symbol_count(void);
struct symbol** Symbol_arrayof(void);

/* 管理状态表的函数 */
int Configcmp(const char*, const char*);
struct state* State_new(void);
void State_init(void);
int State_insert(struct state*,struct config*);
struct state*State_find(struct config*);
struct state**State_arrayof(void);



/**
 * @brief 内存空间申请失败的错误提示处理
 */
void memory_error(void){
    fprintf(stderr,"Out of memory. Aborting...\n");
    exit(1);
}

static int nDefine = 0;     ///< 选项D=..指定的宏的数目
static char** azDefine = 0; ///< 储存选项D=..指定的宏名称
/**
 * @brief 处理选项D的函数指针,该函数指针最后储存在选项D的附加参数arg里.
 * 该函数主要用来启用语法文件(以.y为后缀)里%ifdef和%ifndef定义的宏.
 * @param z 宏名称
 */
static void handle_D_option(char*z){
    char** paz;
    nDefine++; // 每一个handle_D_option处理一个D=..指定的宏,因此函数开头就把nDefine++.
    azDefine=(char**)realloc(azDefine, sizeof(azDefine[0])*nDefine);
    // realloc(point,size)在不改变point指向的存储数据的情况下,把point指向的内存空间变成size大小.
    // 如果size小于原来的内存空间大小,就会丢失末尾部分数据;
    // 如果size大于原来的内存空间大小,就相当于给内存扩容,同时也不影响前面保留的数据.
    if (azDefine==0){ // 扩容失败
        fprintf(stderr,"out of memory\n");
        exit(1);
    }
    paz = &azDefine[nDefine-1];// paz存储azDefine[]最后一个元素的地址.注意paz是char**类型的.
    *paz = (char*) malloc(lemonStrlen(z)+1);// 申请*paz指向的内存空间,大小为z的长度加1
    if (*paz==0){ // 申请失败
        fprintf(stderr,"out of memory\n");
        exit(1);
    }
    lemon_strcpy(*paz,z);

    // 后面两步是把字符串*paz里面的'='改为'0'.
    for (z=*paz;*z&&*z!='=';z++){}
    *z=0;
}


static char* user_templatename = NULL; ///< 储存用户自己指定的语法模板文件名
/**
 * @brief 处理选项T的函数指针,该函数指针最后储存在选项T的附加参数arg里.
 * 该函数主要用来指定用户自己定义的模板文件名称(路径),如果没有指定则默认使用lemon自带的lempar.c.
 * @param z 用户指定的模板文件名
 */
static void handle_T_option(char *z){
    user_templatename=(char*)malloc(lemonStrlen(z)+1);//长度多加1,用来储存末尾的'\0'.
    if (user_templatename==0){ // 申请空间失败
        memory_error();
    }
    lemon_strcpy(user_templatename,z); // 把用户指定的模板文件名拷贝到user_templatename里.
}


/**
 * @see main主程序入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数. argv[0]是程序名称,从argv[1]开始才是真正的命令行参数.
 * @return
 */
int main(int argc,char** argv){
    static int version=0;    // 版本号
    static int rpflag=0;     //
    static int basisflag=0;  //
    static int compress=0;   // 压缩action table
    static int quiet=0;      // 不生成报告文件
    static int statistics=0; // 屏幕打印统计信息
    static int mhflag=0;     // 将本来应该分开生成的.h文件并入生成的.c文件里
    static int nolinenosflag=0;//
    static int noResort=0;   //

    // 注意options里每一个元素的第三个参数代表选项附加的参数所在的地址,如果是0代表空地址,没有附加参数;
    // 其余的值都是通过一个存在的地址值强制转换成char*的.
    static struct s_options options[]={
            {OPT_FLAG, "b", (char*)&basisflag, "Print only the basis in report."},
            {OPT_FLAG, "c", (char*)&compress, "Don't compress the action table."},
            {OPT_FSTR, "D", (char*)handle_D_option, "Define an %ifdef macro."},
            {OPT_FSTR, "f", 0, "Ignored.  (Placeholder for -f compiler options.)"},
            {OPT_FLAG, "g", (char*)&rpflag, "Print grammar without actions."},
            {OPT_FSTR, "I", 0, "Ignored.  (Placeholder for '-I' compiler options.)"},
            {OPT_FLAG, "m", (char*)&mhflag, "Output a makeheaders compatible file."},
            {OPT_FLAG, "l", (char*)&nolinenosflag, "Do not print #line statements."},
            {OPT_FSTR, "O", 0, "Ignored.  (Placeholder for '-O' compiler options.)"},
            {OPT_FLAG, "p", (char*)&showPrecedenceConflict,
                    "Show conflicts resolved by precedence rules"},
            {OPT_FLAG, "q", (char*)&quiet, "(Quiet) Don't print the report file."},
            {OPT_FLAG, "r", (char*)&noResort, "Do not sort or renumber states"},
            {OPT_FLAG, "s", (char*)&statistics,
                    "Print parser stats to standard output."},
            {OPT_FLAG, "x", (char*)&version, "Print the version number."},
            {OPT_FSTR, "T", (char*)handle_T_option, "Specify a template file."},
            {OPT_FSTR, "W", 0, "Ignored.  (Placeholder for '-W' compiler options.)"},
            {OPT_FLAG,0,0,0}
    };

    int i;
    int exitcode; // 程序终止返回值
    struct lemon lem;
    struct rule* rp;

    OptInit(argv,options,stderr);  // 初始化选项

    /*
     * 如果用户输入lemon -x,按照前面的处理过程,由于-x选项是OPT_FLAG类型,
     * 并且有'-'的前缀,那么在handleflags()时就已经把v(v=1)赋值给x选项的附加参数arg,
     * 在主程序main()里通过version这个变量可以访问x选项储存的附加参数值,
     * 那么version就应该是1.此时就输出版本信息,然后退出程序(exit(0))就可以了,
     * 不管后面有其他选项都不处理,因为lemon -x就是一个单一的打印版本信息的命令.
     */
    if (version){
        printf("Lemon version 1.0\n");
        exit(0);
    }

    /*
     *  如果前面version=0,就代表用户输入的不是lemon -x,那么就应该处理用户输入的语法文件(.y后缀),
     *  通过OptNArgs()返回应该处理的语法文件个数,如果返回值不为1就错误退出程序,因为只能处理一个语法文件
     */
    if (OptNArgs()!=1){
        fprintf(stderr,"Exactly one filename argument is required.\n");
        exit(1);
    }
    memset(&lem,0, sizeof(lem)); // 初始化lemon结构体空间
    lem.errorcnt=0;              // 如果前面的命令行处理没有问题,这里的errorcnt就应该设置为0

    Strsafe_init();
    Symbol_init();
    State_init();


}

//-----------------------所有函数的实现----------------

/* Option 相关实现 */
static char** argv; ///< 用户的命令行参数
static struct s_options * op; ///< lemon支持的命令行参数选项
static FILE* errstream;   ///< 错误输出流
#define ISOPT(x) ((x)[0]=='-'||(x)[0]=='+'||strchr((x),'=')!=0) ///< 初步判断一个命令行参数是否符合选项格式

static char emsg[]="Command line syntax error: "; ///< 错误提示字符串的开头

/**
 * 在第n+1个(n从0开始)命令行参数的第k个字符下方标出出错的地方
 * @param n   命令行参数的索引下标
 * @param k   命令行参数出错的字符位置
 * @param err 错误输出流
 */
static void errline(int n,int k,FILE*err){
    int spcnt,i;// spcnt 计算出现错误的字符距离屏幕左侧的距离
    if (argv[0]) fprintf(err,"%s",argv[0]);// 打印第一个命令行参数(即程序名称lemon)
    spcnt=lemonStrlen(argv[0])+1; // 两个命令行参数之间有一个空格,所以要加1
    for(i=1;i<n&&argv[i];i++){
        fprintf(err," %s",argv[i]); // 打印出错字符之前的命令行参数,
        spcnt+=lemonStrlen(argv[i])+1;
    }
    spcnt+=k;
    for(;argv[i];i++) fprintf(err," %s",argv[i]);// 打印剩下的命令行参数
    if (spcnt<20){ // 如果spcnt<20,以^-- here的形式标注错误
        fprintf(err,"\n%*s^-- here\n",spcnt,"");
    }else{         // 如果spcnt>=20,以here --^的形式标注错误
        fprintf(err,"\n%*shere --^\n",spcnt-7,"");
    }
}
/**
 * @brief 处理命令行参数中带前缀"+"或"-"的选项,比如lemon -b. 这种参数是没有赋值的.
 * @param i   命令行参数在argv里的索引
 * @param err 错误输出流
 * @return    返回出现错误的个数
 */
static int handleflags(int i,FILE*err) {
    int v;
    int errcnt = 0;  // 统计错误个数
    int j;
    for (j = 0; op[j].label; j++) {
        // 遍历lemon支持的选项数组,若找出一个与命令行参数的值匹配的就立即退出.
        // 因此for循环退出后,若op[j].label不为0,则代表匹配成功.
        if (strncmp(&argv[i][1], op[j].label, lemonStrlen(op[j].label)) == 0) break;
    }
    v=((argv[i][0]=='-')?1:0); // 若选项带"-"前缀则v为1,若前缀为"+"则v为0
    if (op[j].label==0){// 匹配不成功
        if (err){
            fprintf(err,"%sundefined option.\n",emsg);
            errline(i,1,err);
        }
        errcnt++;
    }else if(op[j].arg==0){
        // 如果选项没有相应的附加参数地址,忽略该选项
    }else if(op[j].type==OPT_FLAG){
        // 如果选项类型是OPT_FLAG,就把v的值写入这个选项的附加参数地址所在的地方.
        // 同时选项的附加参数arg此时被强制转换为int*,说明此时arg指针指向的是一个int值
        *((int*)op[j].arg) = v;
    }else if(op[j].type==OPT_FFLAG){
        // 如果选项类型是OPT_FFLAG,首先把选项附加的参数指针arg强制转换为
        // (void(*)(int))类型的函数指针,然后把v的值作为arg指向的函数的实参传入,
        // 相当于直接调用arg指针指向的函数.所以这一步是一个函数调用过程而不是一个存值的过程.
        // 注意后面代码的阅读顺序应该是: (*((void(*)(int))(op[j].arg)))(v);
        (*(void(*)(int))(op[j].arg))(v);
    }else if(op[j].type==OPT_FSTR){
        // 如果选项类型是OPT_FSTR,首先把选项附加的参数指针arg强制转换为
        // (void(*)(char*))类型的函数指针,然后传递&argv[i][2]调用这个函数指针指向的函数.
        (*(void(*)(char*))(op[j].arg))(&argv[i][2]);
    }else{
        if (err){
            fprintf(err,"%smissing argument on switch.\n",emsg);
            errline(i,1,err);
        }
        errcnt++;
    }
    return errcnt;
}

/**
 * @brief 处理命令行参数中带赋值符号"="的选项,比如lemon D=MACROS.
 * @param i   命令行参数在argv里的索引
 * @param err 错误输出流
 * @return    返回出现错误的个数
 */
static int handleswitch(int i,FILE *err){
    int lv = 0;      // 提取"="后面的整数
    double dv = 0.0; // 提取"="后面的浮点数
    char* sv=0;      // 提取"="后面的字符串

    char* end;
    // 提取"="后面的部分以后,end指向的地址.
    // 比如"D=123\0",调用strtol(startPoint,&end,0)提取后面的整数:
    // for(end=startPoint;*end!='\0';end++){is(!isdigit(*end)) break;}
    // 如果正常提取,最后*end应该为"\0".但如果提取后*end!="\0",说明参数错误.

    char* cp;
    // 命令行参数"="的地址,可以利用*cp=0切断命令行参数

    int j;
    int errcnt=0;
    cp=strchr(argv[i],'='); // cp保存命令行参数中间'='的地址
    assert(cp!=0);
    *cp=0; // 改变命令行参数中间的'='为'0',中断命令行参数,这样后面的argv[i]就只保留前面的部分,才能与option的label进行比较.
    for (j=0;op[j].label;j++){
        if (strcmp(argv[i],op[j].label)==0) break; // 找到匹配的选项,退出循环
    }
    *cp='='; // 恢复命令行参数中间的'='
    if (op[j].label==0){ // 如果与lemon支持的选项不匹配,说明出错
        if (err){
            fprintf(err,"%sundefined option.\n",emsg);
            errline(i,0,err);
        }
        errcnt++;
    }else{
        cp++; // cp指向"="后面部分的起始地址
        switch (op[j].type){
            case OPT_FLAG:
            case OPT_FFLAG: // 与FLAG相关的选项类型不允许"="赋值的情形,直接错误
                if(err){
                    fprintf(err,"%soption requires an argument.\n",emsg);
                    errline(i,0,err);
                }
                errcnt++;
                break;
            case OPT_DBL:
            case OPT_FDBL:
                dv=strtod(cp,&end); // 提取"="后面的浮点数
                if (*end){ // 如果提取后*end不等于'\0'则错误
                    if (err){
                        fprintf(err,"%sillegal character in floating-point argument.\n",emsg);
                        errline(i,(int)((char*)end-(char*)argv[i]),err);
                    }
                    errcnt++;
                }
                break;
            case OPT_INT:
            case OPT_FINT:
                lv=strtol(cp,&end,0); // 提取"="后面的整数,strtol()第三个参数是进制选择,取0用十进制
                if (*end){ // 如果提取后*end不等于'\0'则错误
                    if (err){
                        fprintf(err,"%sillegal character in integer argument.\n",emsg);
                        errline(i,(int)((char*)end-(char*)argv[i]),err);
                    }
                    errcnt++;
                }
                break;
            case OPT_STR:
            case OPT_FSTR:
                sv=cp;  // 如果是要提取后面的字符串,直接提取后退出
                break;
        }
        switch (op[j].type){
            case OPT_FLAG:
            case OPT_FFLAG:
                break;
            case OPT_DBL: // OPT_DBL类型的选项直接把提取的浮点数dv赋值给选项附加的参数op[j].arg
                *(double*)(op[j].arg) =dv;
                break;
            case OPT_FDBL:
                // OPT_FDBL类型的选项需要先把op[j].arg的指针强制转换为(void(*)(double))类型的函数指针,
                // 然后解开这个函数指针(用*)变成函数,然后传递实参dv,相当于调用一次op[j].arg指针指向的函数.
                (*(void(*)(double))(op[j].arg))(dv);
                // 注意上面一句代码应该这样阅读:(*((void(*)(double))(op[j].arg)))(dv);
                break;
            case OPT_INT: // 语义同double类型处理
                *(int*)(op[j].arg)=lv;
                break;
            case OPT_FINT: // 语义同double类型的处理
                (*(void(*)(int))(op[j].arg))(lv);
                break;
            case OPT_STR: //同上分析
                *(char**)(op[j].arg)=sv;
                break;
            case OPT_FSTR: // 同上分析
                (*(void(*)(char*))(op[j].arg))(sv);
                break;
        }
    }
    return errcnt;
}
/**
 * @brief 选项初始化函数
 * @param a   用户输入的命令行参数
 * @param o   main()函数定义的s_options结构体数组,提供了lemon支持的所有选项
 * @param err 标准错误输出流
 * @return 用户的命令行参数出现错误的个数
 */
int OptInit(char**a,struct s_options*o,FILE*err){
    int errcnt = 0; // 错误个数统计
    argv=a;
    op=o;
    errstream=err;
    if ( argv && *argv && op ){ // 满足argv,*argv和op不为空才能继续处理
        int i;
        for (i=1;argv[i];i++){ // 遍历用户的命令行参数,从argv[1]开始
            if (argv[i][0]=='+'||argv[i][0]=='-'){
                errcnt+=handleflags(i,err);
            }else if(strchr(argv[i],'=')){
                errcnt+=handleswitch(i,err);
            }
        }
    }
    if (errcnt>0){ // 如果错误个数大于0,打印相关错误信息以及帮助信息
        fprintf(err,"Valid command line options for \"%s\" are:\n",*a);
        OptPrint(); // 打印lemon帮助信息
        exit(1);    // 错误终止程序
    }
    return 0; // 如果没有错误返回0
}

/**
 * @brief 统计命令行参数中非选项参数(不带'+'/'-'/'=')以及dashdash("--")的个数.
 * 所谓非选项参数即命令行参数中语法文件的名称(以.y为后缀),如果返回值为1则说明语法文件只有一个,
 * 就是正确返回值;如果返回值大于1,说明要处理的语法文件多于一个,就不符合程序要求.
 * 比较奇怪的是,这里的返回值还包含了"--"的个数,就不太清楚其原因???
 * @return 返回非选项参数以及"--"的个数(可以认为是语法文件的个数),按照程序要求正确的返回值是1.
 */
int OptNArgs(void){
    int cnt = 0;
    int dashdash = 0;
    int i;
    if (argv!=0 && argv[0]!=0){ // 命令行参数不能为空
        for (i=1; argv[i];i++){ // 从argv[1]开始遍历命令行参数,argv[0]是程序名称,不用处理.
            if (dashdash || !ISOPT(argv[i])) cnt++;
            if (strcmp(argv[i],"--")==0) dashdash=1;
        }
    }
    return cnt;
}
/**
 *
 * @param n
 * @return
 */
char* OptArg(int n){

}
/**
 *
 * @param n
 */
void OptErr(int n){

}
/**
 * @brief 打印lemon的帮助信息(所有支持选项及其功能)
 */
void OptPrint(void){
    int i;
    int max, len;
    max = 0;
    for(i=0; op[i].label; i++){
        len = lemonStrlen(op[i].label) + 1;
        switch( op[i].type ){
            case OPT_FLAG:
            case OPT_FFLAG:
                break;
            case OPT_INT:
            case OPT_FINT:
                len += 9;       /* length of "<integer>" */
                break;
            case OPT_DBL:
            case OPT_FDBL:
                len += 6;       /* length of "<real>" */
                break;
            case OPT_STR:
            case OPT_FSTR:
                len += 8;       /* length of "<string>" */
                break;
        }
        if( len>max ) max = len;
    }
    for(i=0; op[i].label; i++){
        switch( op[i].type ){
            case OPT_FLAG:
            case OPT_FFLAG:
                fprintf(errstream,"  -%-*s  %s\n",max,op[i].label,op[i].message);
                break;
            case OPT_INT:
            case OPT_FINT:
                fprintf(errstream,"  -%s<integer>%*s  %s\n",op[i].label,
                        (int)(max-lemonStrlen(op[i].label)-9),"",op[i].message);
                break;
            case OPT_DBL:
            case OPT_FDBL:
                fprintf(errstream,"  -%s<real>%*s  %s\n",op[i].label,
                        (int)(max-lemonStrlen(op[i].label)-6),"",op[i].message);
                break;
            case OPT_STR:
            case OPT_FSTR:
                fprintf(errstream,"  -%s<string>%*s  %s\n",op[i].label,
                        (int)(max-lemonStrlen(op[i].label)-8),"",op[i].message);
                break;
        }
    }
}

/// \brief 处理字符串时进行数据储存的结构体
struct s_x1{
    int size;   ///< 存储容量(最大允许存储的数量,必须是2的指数倍或者等于1)
    int count;  ///< 当前实际存储的数据数量
    struct s_x1node * tbl; ///< 储存s_x1node节点的数组
    struct s_x1node ** ht; ///< 对s_x1node节点进行管理的哈希表,方便搜索.注意是一个节点指针数组
};
/// \brief 辅助s_x1存储的节点结构,数据指针data储存在节点里
typedef struct s_x1node{
    const char* data; ///< 真实数据
    struct s_x1node * next;  ///< 下一个节点
    struct s_x1node ** from; ///<
} x1node;

static struct s_x1 * x1a; ///< 全局的s_x1结构体实例
/**
 * @brief 对s_x1的唯一实例x1a进行内存分配
 * @see 这个函数的内存分配技巧比较高,分配时节点与节点指针是紧凑储存的,
 * 这样一来不浪费内存,二来方便管理以及加速搜索(哈希表与实际储存数据放在一起搜索更快).
 * 首先指定数据最多支持1024条,然后创建一整块内存并清空数据,代码是:
 * x1a->tbl=(s_x1node*)calloc(1024,sizeof(s_x1node)+sizeof(s_x1node*));
 * 画成图像: |s_x1node+s_x1node*大小 s_x1node+s_x1node*大小 .... (连续1024块)|
 * 但是注意上面那块内存空间整一块是空白的,实际储存的时候并不是s_x1node跟着一个s_x1node*
 * (只是为了描述分配情况这样画),因为成员tbl是一个s_x1node数组,数组元素肯定是紧凑在一起的,
 * 所以上面那一片空白内存前面的1024*sizeof(s_x1node)大小的部分给tbl[]管;
 * 后面1024*sizeof(s_x1node*)大小的部分给ht[]管,所以实际的分配是:
 * | .....(1024*sizeof(s_x1node)大小归tbl[])..... \ .....(1024*sizeof(s_x1node*)大小归ht[]).....|
 */
void Strsafe_init(void){
    if(x1a) return; // 只初始化内存一次
    x1a=(struct s_x1*)malloc(sizeof(struct s_x1));
    if (x1a){
        x1a->size=1024; // 必须是2的指数幂
        x1a->count=0;   // 当前实际数据数量为0
        x1a->tbl=(x1node*)calloc(1024, sizeof(x1node)+ sizeof(x1node*));
        if (x1a->tbl==0){ // tbl申请空间失败
            free(x1a); // 释放x1a已经占有的空间
            x1a=0;     // 将x1a设置为空指针,避免x1a变成野指针
        }else{
            int i;
            x1a->ht=(x1node**)&(x1a->tbl[1024]); // 把tbl[1024]所在的地址设置为ht[]的首地址,那么后面的部分就由ht[]管理
            for (i=0;i<1024;i++) x1a->ht[i]=0; // 将ht[]清零
        }
    }
}

/// \brief 类似于s_x1的结构
struct s_x2{
    int size; ///< 容量,必须是2的指数幂
    int count;///< 实际数量
    struct s_x2node *tbl;///< 储存s_x2node的数组
    struct s_x2node **ht;///< 储存s_x2node*的数组,作为哈希表方便搜索
};
/// \brief 类似于s_x1node,区别在于s_x2node是来储存symbol(符号)的
typedef struct s_x2node{
    struct symbol *data;  ///< symbol(符号)数组
    const char *key;      ///< 储存各个symbol的名称
    struct s_x2node*next; ///< 下一个节点
    struct s_x2node**from;///<
}x2node;

struct s_x2 *x2a; ///< 全局的s_x2实例
/**
 * @brief 给s_x2实例指针x2a申请内存空间
 * @see 具体实现与Strsafe_init()类似
 */
void Symbol_init(void){
    if (x2a) return;
    x2a=(struct s_x2*)malloc(sizeof(struct s_x2));
    if (x2a){
        x2a->size=128;
        x2a->count=0;
        x2a->tbl=(x2node*)calloc(128, sizeof(x2node)+ sizeof(x2node*));
        if (x2a->tbl==0){
            free(x2a);
            x2a=0;
        }else{
            int i;
            x2a->ht=(x2node**)&(x2a->tbl[128]);
            for (i=0;i<128;i++) x2a->ht[i]=0;
        }
    }
}

/// \brief 类似于s_x1与s_x2
struct s_x3{
    int size;
    int count;
    struct s_x3node *tbl;
    struct s_x3node **ht;
};
/// \brief 类似于s_x1node与s_x2node,用来储存state
typedef struct s_x3node{
    struct state* data;
    struct config* key;
    struct s_x3node* next;
    struct s_x3node** from;
}x3node;
static struct s_x3*x3a; ///< 全局唯一的s_x3实例

/**
 * @brief 对x3a的内存初始化
 * @see 具体实现类似于Symbol_init()
 */
void State_init(void){
    if (x3a) return;
    x3a=(struct s_x3*)malloc(sizeof(struct s_x3));
    if (x3a){
        x3a->size=128;
        x3a->count=0;
        x3a->tbl=(x3node*)calloc(128, sizeof(x3node)+ sizeof(x3node*));
        if (x3a->tbl==0){
            free(x3a);
            x3a=0;
        } else{
            int i;
            x3a->ht=(x3node**)&(x3a->tbl[128]);
            for (i=0;i<128;i++) x3a->ht[i]=0;
        }
    }
}
