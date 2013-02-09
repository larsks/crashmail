#include <stdlib.h>
#include <string.h>

#include "shared/types.h"
#include "shared/expr.h"

#include <oslib/os.h>
#include <oslib/osmem.h>

struct expr *expr_get(struct expr *expr,int *errpos,char **errstr)
{
   int c,d,len,start;
   bool hasnot,hasquote,stop;
   struct expr *res;

   hasnot=FALSE;

   c=expr->parsepos;

   /* Skip leading whitespace */

   while(expr->buf[c]==' ')
      c++;

   /* Check for NOT */

   if(strnicmp(&expr->buf[c],"NOT ",4)==0)
   {
      c+=4;
      hasnot=TRUE;
   }

   /* Check if parentheses */

   if(expr->buf[c]=='(')
   {
      int nump;

      nump=1;
      d=c+1;

      while(nump!=0 && expr->buf[d]!=0)
      {
         if(expr->buf[d]=='(') nump++;
         if(expr->buf[d]==')') nump--;

         d++;
      }

      if(nump == 0)
      {
         start=c+1;
         len=d-c-2;

         if(!(res=osAlloc(sizeof(struct expr))))
         {
            *errpos=expr->offset;
            *errstr="Out of memory";
            return(NULL);
         }

         if(!(res->buf=osAlloc(len+10)))
         {
            osFree(res);
            *errpos=expr->offset;
            *errstr="Out of memory";
            return(NULL);
         }

         res->parsepos=0;
         res->offset=expr->offset+start;
         res->buf[0]=0;

         if(hasnot)
            strcat(res->buf,"NOT ");

         strncat(res->buf,&expr->buf[start],len);

         expr->parsepos=d;

         return(res);
      }
      else
      {
         *errpos=expr->offset+c;
         *errstr="No matching parenthesis found";
         return(NULL);
      }
   }

   d=c;

   stop=FALSE;
   hasquote=FALSE;

   while(!stop)
   {
      if(expr->buf[d] == 0)
         stop=TRUE;

      if(expr->buf[d] == ' ' && !hasquote)
         stop=TRUE;

      if(expr->buf[d] == '"' && hasquote)
         stop=TRUE;

      if(expr->buf[d] == '"')
         hasquote=TRUE;

      if(!stop)
         d++;
   }

   if(expr->buf[d]=='"')
      d++;

   start=c;
   len=d-c;

   if(!(res=osAlloc(sizeof(struct expr))))
   {
      *errpos=expr->offset;
      *errstr="Out of memory";
      return(NULL);
   }

   if(!(res->buf=osAlloc(len+10)))
   {
      osFree(res);
      *errpos=expr->offset;
      *errstr="Out of memory";
      return(NULL);
   }

   res->parsepos=0;
   res->offset=expr->offset+start;
   res->buf[0]=0;

   if(hasnot)
      strcat(res->buf,"NOT ");

   strncat(res->buf,&expr->buf[start],len);

   expr->parsepos=d;

   return(res);
}

struct expr *expr_getrest(struct expr *expr,int *errpos,char **errstr)
{
   int c,d,start,len;
   struct expr *res;

   c=expr->parsepos;

   /* Skip leading whitespace */

   while(expr->buf[c]==' ')
      c++;

   d=c;

   while(expr->buf[d]!=0)
      d++;

   start=c;
   len=d-c;

   if(!(res=osAlloc(sizeof(struct expr))))
   {
      *errpos=expr->offset;
      *errstr="Out of memory";
      return(NULL);
   }

   if(!(res->buf=osAlloc(len+10)))
   {
      osFree(res);
      *errpos=expr->offset;
      *errstr="Out of memory";
      return(NULL);
   }

   res->parsepos=0;
   res->offset=expr->offset+start;
   res->buf[0]=0;

   strncat(res->buf,&expr->buf[start],len);

   expr->parsepos=d;

   return(res);
}

struct expr *expr_makeexpr(char *str)
{
   struct expr *expr;

   if(!(expr=osAllocCleared(sizeof(struct expr))))
   {
      return(NULL);
   }

   if(!(expr->buf=osAlloc(strlen(str)+1)))
   {
      osFree(expr);
      return(NULL);
   }

   strcpy(expr->buf,str);

   return(expr);
}

void expr_free(struct expr *expr)
{
   osFree(expr->buf);
   osFree(expr);
}

int expr_eval(struct expr *expr,int (*eval)(char *str,int *errpos,char **errstr),int *errpos,char **errstr)
{
   struct expr *e1,*e2,*e3;

   e1=expr_get(expr,errpos,errstr);

   if(!e1)
   {
      return(-1);
   }

   e2=expr_get(expr,errpos,errstr);

   if(!e2)
   {
      expr_free(e1);
      return(-1);
   }

   e3=expr_getrest(expr,errpos,errstr);

   if(!e3)
   {
      expr_free(e1);
      expr_free(e2);
      return(-1);
   }

   if(e2->buf[0] == 0)
   {
      /* Only one expression */

      if(strcmp(e1->buf,expr->buf)==0)
      {
         /* No more simplification possible */

         if(strnicmp(e1->buf,"NOT ",4)==0)
         {
            int res;

            strcpy(e1->buf,&e1->buf[4]);
            e1->parsepos=0;

            res=expr_eval(e1,eval,errpos,errstr);

            if(res == 1) res=0;
            else if(res == 0) res=1;

/* printf("NOT |%s|: %d\n",e1->buf,res); */

            expr_free(e1);
            expr_free(e2);
            expr_free(e3);

            return(res);
         }
         else
         {
            int res,tmperrpos;

            res=(*eval)(e1->buf,&tmperrpos,errstr);

            if(res == -1)
               *errpos=e1->offset+tmperrpos;

/* printf("EVAL: |%s|: %d\n",e1->buf,res); */

            expr_free(e1);
            expr_free(e2);
            expr_free(e3);

            return(res);
         }
      }
      else
      {
         int res;

         res=expr_eval(e1,eval,errpos,errstr);

/* printf("|%s|: %d\n",e1->buf,res); */

         expr_free(e1);
         expr_free(e2);
         expr_free(e3);

         return (res);
      }
   }

   if(stricmp(e2->buf,"AND")==0)
   {
      int r1,r2,res;

      if(e3->buf[0] == 0)
      {
         *errstr="Expected second expression after AND";
         *errpos=e3->offset;

         expr_free(e1);
         expr_free(e2);
         expr_free(e3);

         return(-1);
      }

      r1=expr_eval(e1,eval,errpos,errstr);

      if(r1 == -1)
      {
         expr_free(e1);
         expr_free(e2);
         expr_free(e3);
         return(-1);
      }

      r2=expr_eval(e3,eval,errpos,errstr);

      if(r2 == -1)
      {
         expr_free(e1);
         expr_free(e2);
         expr_free(e3);
         return(-1);
      }

      res=0;

      if(r1 && r2)
         res=1;

/* printf("|%s| AND |%s|: %d\n",e1->buf,e3->buf,res); */

      expr_free(e1);
      expr_free(e2);
      expr_free(e3);

      return(res);
   }
   else if(stricmp(e2->buf,"OR")==0)
   {
      int r1,r2,res;

      if(e3->buf[0] == 0)
      {
         *errstr="Expected second expression after OR";
         *errpos=e3->offset;

         expr_free(e1);
         expr_free(e2);
         expr_free(e3);

         return(-1);
      }

      r1=expr_eval(e1,eval,errpos,errstr);

      if(r1 == -1)
      {
         expr_free(e1);
         expr_free(e2);
         expr_free(e3);

         return(-1);
      }

      r2=expr_eval(e3,eval,errpos,errstr);

      if(r2 == -1)
      {
         expr_free(e1);
         expr_free(e2);
         expr_free(e3);

         return(-1);
      }

      res=0;

      if(r1 || r2)
         res=1;

/* printf("|%s| OR |%s|: %d\n",e1->buf,e3->buf,res); */

      expr_free(e1);
      expr_free(e2);
      expr_free(e3);

      return(res);
   }
   else
   {
      *errstr="Expected AND or OR";
      *errpos=e2->offset;

      expr_free(e1);
      expr_free(e2);
      expr_free(e3);

      return(-1);
   }
}

