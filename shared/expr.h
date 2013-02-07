struct expr
{
   uchar *buf;
   int parsepos;
   int offset;
};

int expr_eval(struct expr *expr,int (*eval)(uchar *str,int *errpos,uchar **errstr),int *errpos,uchar **errstr);
struct expr *expr_makeexpr(uchar *str);
void expr_free(struct expr *expr);

