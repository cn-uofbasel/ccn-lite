//
// Created by Balázs Faludi on 27.06.17.
//

#ifndef CCNL_NFN_PARSE_H
#define CCNL_NFN_PARSE_H

#define LAMBDACHAR '@'

#define term_is_var(t)     (!(t)->m)
#define term_is_app(t)     ((t)->m && (t)->n)
#define term_is_lambda(t)  (!(t)->n)


struct ccnl_lambdaTerm_s {
    char *v;
    struct ccnl_lambdaTerm_s *m, *n;
    // if m is 0, we have a var  v
    // is both m and n are not 0, we have an application  (M N)
    // if n is 0, we have a lambda term  @v M
};

struct ccnl_lambdaTerm_s*
ccnl_lambdaStrToTerm(int lev, char **cp, int (*prt)(char* fmt, ...));

void
ccnl_lambdaFreeTerm(struct ccnl_lambdaTerm_s *t);

int
ccnl_lambdaTermToStr(char *cfg, struct ccnl_lambdaTerm_s *t, char last);

#endif //CCNL_NFN_PARSE_H
