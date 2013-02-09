struct expr
{
   char *buf;
   int parsepos;
   int offset;
};

int expr_eval(struct expr *expr,int (*eval)(char *str,int *errpos,char **errstr),int *errpos,char **errstr);
struct expr *expr_makeexpr(char *str);
void expr_free(struct expr *expr);

