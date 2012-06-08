/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004, 2008, 2010, 2011, 2012 Antti-Juhani Kaijanaho

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <ctype.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include "atom.h"
#include "fsaf.h"
#include "msg.h"
#include "util.h"
#include "predicate.h"
#include "strutil.h"
#include "version.h"

typedef bool (*eval_t)(struct predicate *, para_t * para);

struct predicate_vtbl {
        bool (*eval)(struct predicate *, para_t *);
        void (*print)(struct predicate *, size_t indent);
};

struct predicate {
        const struct predicate_vtbl *vtbl;
};

static void print(struct predicate *p, size_t indent)
{
        p->vtbl->print(p, indent);
}

struct unary_predicate {
        struct predicate super;
        struct predicate *rand;
};

struct binary_predicate {
        struct predicate super;
        struct predicate *lrand;
        struct predicate *rrand;
};

struct atomary_predicate {
        struct predicate super;
        struct atom *atom;
};

static bool eval_AND(struct predicate *base_p, para_t * para)
{
        struct binary_predicate *p = (struct binary_predicate *)base_p;
        if (!does_para_satisfy(p->lrand, para)) return false;
        return does_para_satisfy(p->rrand, para);
}

static void print_AND(struct predicate *base_p, size_t indent)
{
        struct binary_predicate *p = (struct binary_predicate *)base_p;
        for (size_t i = 0; i < indent; i++) putchar(' ');
        puts("AND");
        print(p->lrand, indent+1);
        print(p->rrand, indent+1);
}

static bool eval_OR(struct predicate *base_p, para_t * para)
{
        struct binary_predicate *p = (struct binary_predicate *)base_p;
        if (does_para_satisfy(p->lrand, para)) return true;
        return does_para_satisfy(p->rrand, para);
}

static void print_OR(struct predicate *base_p, size_t indent)
{
        struct binary_predicate *p = (struct binary_predicate *)base_p;
        for (size_t i = 0; i < indent; i++) putchar(' ');
        puts("OR");
        print(p->lrand, indent+1);
        print(p->rrand, indent+1);
}

static bool eval_NOT(struct predicate *base_p, para_t * para)
{
        struct unary_predicate *p = (struct unary_predicate *)base_p;
        return !does_para_satisfy(p->rand, para);
}

static void print_NOT(struct predicate *base_p, size_t indent)
{
        struct unary_predicate *p = (struct unary_predicate *)base_p;
        for (size_t i = 0; i < indent; i++) putchar(' ');
        puts("NOT");
        print(p->rand, indent+1);
}


static bool eval_ATOM(struct predicate *base_p, para_t * para)
{
        struct atomary_predicate *p = (struct atomary_predicate *)base_p;
        return atom_verify(p->atom, para);
}

static void print_ATOM(struct predicate *base_p, size_t indent)
{
        struct atomary_predicate *p = (struct atomary_predicate *)base_p;
        char ind[indent+1];
        for (size_t i = 0; i < indent; i++) ind[i] = ' ';
        ind[indent] = '\0';
        printf("%sATOM", ind);
        printf("%s field_name = %s\n", ind, p->atom->field_name);
        printf("%s mode = %i\n", ind, p->atom->mode);
        printf("%s ignore_case = %i\n", ind, p->atom->ignore_case);
        printf("%s whole_pkg = %i\n", ind, p->atom->whole_pkg);
        printf("%s pat = %s\n", ind, p->atom->pat);
}

struct predicate *binary_predicate(const struct predicate_vtbl *vtbl,
                                   struct predicate *pl, struct predicate *pr){
        struct binary_predicate *rv = malloc(sizeof *rv);
        if (rv == 0) enomem(0);
        rv->super.vtbl = vtbl;
        rv->lrand= pl;
        rv->rrand = pr;
        return &rv->super;
}
struct predicate *predicate_AND(struct predicate *pl, struct predicate *pr)
{
        static const struct predicate_vtbl vtbl =
                { .eval = eval_AND, .print = print_AND };
        return binary_predicate(&vtbl, pl, pr);
}
struct predicate *predicate_OR(struct predicate *pl, struct predicate *pr)
{
        static const struct predicate_vtbl vtbl =
                { .eval = eval_OR, .print = print_OR };
        return binary_predicate(&vtbl, pl, pr);
}
struct predicate *predicate_NOT(struct predicate *p)
{
        static const struct predicate_vtbl vtbl =
                { .eval = eval_NOT, .print = print_NOT };
        struct unary_predicate *rv = malloc(sizeof *rv);
        if (rv == 0) enomem(0);
        rv->super.vtbl = &vtbl;
        rv->rand = p;
        return &rv->super;
}
struct predicate *predicate_ATOM(struct atom *at)
{
        static const struct predicate_vtbl vtbl =
                { .eval = eval_ATOM, .print = print_ATOM };
        struct atomary_predicate *rv = malloc(sizeof *rv);
        if (rv == 0) enomem(0);
        rv->super.vtbl = &vtbl;
        rv->atom = at;
        return &rv->super;
}


bool check_predicate(struct predicate * p __attribute__((unused)))
{
        // static checking of predicate
        // currently no operation
        return true;
}

bool does_para_satisfy(struct predicate * p, para_t * para)
{
        return p->vtbl->eval(p, para);
}

void predicate_print(struct predicate *p)
{
        print(p, 0);
}
