#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include "nand.h"

#define INITIAL_OUTPUTS 4

typedef struct input {
    union {
        bool const *signal;
        nand_t *gate;
    } handler;
    bool is_gate;
    unsigned output_no;
    bool is_occupied;
} input_t;

typedef struct output {
    nand_t *gate;
    unsigned input_no;
} output_t;

typedef struct nand {
    unsigned input_num;
    input_t *inputs;

    unsigned output_num;
    unsigned output_capacity;
    output_t *outputs;

    unsigned eval_num;
    bool is_checked;
    bool value;
    ssize_t critical_lenght;
} nand_t;

static void output_remove(nand_t *base, unsigned k){
    base->outputs[k] = base->outputs[base->output_num - 1];
    base->outputs[k].gate->inputs[base->outputs[k].input_no].output_no = k;
    base->output_num--;
}

static int output_append(nand_t *base, nand_t* gate, unsigned num){
    if(base->output_capacity == base->output_num){
        base->outputs = (output_t *) realloc(base->outputs,
                base->output_capacity * 2 * sizeof(output_t));
        base->output_capacity *= 2;
    }

    if(base->outputs == NULL) return -1;

    base->outputs[base->output_num].gate = gate;
    base->outputs[base->output_num].input_no = num;
    base->output_num++;
    return base->output_num - 1;
}

nand_t *nand_new(unsigned n) {
    nand_t *gate = (nand_t *)malloc(sizeof(nand_t));
    if (gate == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    gate->inputs = (input_t *)calloc(n, sizeof(input_t));
    if (gate->inputs == NULL){
        free(gate);
        errno = ENOMEM;
        return NULL;
    }

    gate->outputs = (output_t *) calloc(INITIAL_OUTPUTS, sizeof(output_t));
    if(gate->outputs == NULL){
        free(gate->inputs);
        free(gate);
        errno = ENOMEM;
        return NULL;
    }

    gate->input_num = n;
    gate->output_capacity = INITIAL_OUTPUTS;
    gate->output_num = 0;
    gate->eval_num = 0;
    gate->is_checked = 0;
    gate->value = 0;
    gate->critical_lenght = 0;
    return gate;
}

void nand_delete(nand_t *g){
    if(g == NULL)
        return;

    for(unsigned i = 0; i < g->input_num; i++)
        if(g->inputs[i].is_gate && g->inputs[i].is_occupied)
            output_remove(g->inputs[i].handler.gate, g->inputs[i].output_no);

    for(unsigned i = 0; i < g->output_num; i++)
        g->outputs[i].gate->inputs[g->outputs[i].input_no].is_occupied = false;

    free(g->inputs);
    free(g->outputs);
    free(g);
}

int nand_connect_nand(nand_t *g_out, nand_t* g_in, unsigned k){
    if(g_in == NULL || g_out == NULL){
        errno = EINVAL;
        return -1;
    }

    if(k >= g_in->input_num){
        errno = EINVAL;
        return -1;
    }

    int result = output_append(g_out, g_in, k);

    if(result == -1){
        errno = ENOMEM;
        return -1;
    }

    if(g_in->inputs[k].is_occupied && g_in->inputs[k].is_gate){
        output_remove(g_in->inputs[k].handler.gate, g_in->inputs[k].output_no);
    }

    g_in->inputs[k].output_no = result;
    g_in->inputs[k].is_gate = true;
    g_in->inputs[k].is_occupied = true;
    g_in->inputs[k].handler.gate = g_out;

    return 0;
}

int nand_connect_signal(bool const *s, nand_t *g_in, unsigned k){
    if(g_in == NULL || s == NULL){
        errno = EINVAL;
        return -1;
    }

    if(k >= g_in->input_num){
        errno = EINVAL;
        return -1;
    }

    if(g_in->inputs[k].is_occupied && g_in->inputs[k].is_gate){
        output_remove(g_in->inputs[k].handler.gate, g_in->inputs[k].output_no);
    }

    g_in->inputs[k].is_gate = false;
    g_in->inputs[k].is_occupied = true;
    g_in->inputs[k].handler.signal = s;

    return 0;
}

ssize_t nand_fan_out(nand_t const *g){
    if(g == NULL){
        errno = EINVAL;
        return -1;
    }
    return g->output_num;
}

void* nand_input(nand_t const *g, unsigned k){
    if(g == NULL){
        errno = EINVAL;
        return NULL;
    }

    if(k >= g->input_num){
        errno = EINVAL;
        return NULL;
    }

    if(!g->inputs[k].is_occupied){
        errno = 0;
        return NULL;
    }

    return (void *) g->inputs[k].handler.gate;
}

nand_t* nand_output(nand_t const *g, ssize_t k){
    return g->outputs[k].gate;
}

static ssize_t nand_evaluate_single(nand_t *g, unsigned eval_no){
    if(g == NULL)
        return -1;
    if(g->eval_num == eval_no){
        if(!g->is_checked)
            return -1;
        return g->critical_lenght;
    }
    g->eval_num = eval_no;
    g->is_checked = false;
    g->critical_lenght = 0;
    bool result = 1;
    for(unsigned i = 0; i < g->input_num; i++){
        if(!g->inputs[i].is_occupied)
            return -1;
        if(g->inputs[i].is_gate){
            ssize_t critical_lenght = nand_evaluate_single(g->inputs[i].handler.gate, eval_no);
            if(critical_lenght == -1)
                return -1;
            if(critical_lenght >= g->critical_lenght)
                g->critical_lenght = critical_lenght + 1;
            result &= g->inputs[i].handler.gate->value;
        }else{
            result &= *g->inputs[i].handler.signal;
            if(g->critical_lenght == 0){
                g->critical_lenght = 1;
            }
        }
    }
    g->value = !result;
    g->is_checked = true;
    return g->critical_lenght;
}

ssize_t nand_evaluate(nand_t **g, bool *s, size_t m){
    if(g == NULL || s == NULL || m == 0){
        errno = EINVAL;
        return -1;
    }

    static int eval_no = 0;
    eval_no++;

    ssize_t max_path = 0;
    for(size_t i = 0; i < m; i++){
        ssize_t result = nand_evaluate_single(g[i], eval_no);
        if(result == -1){
            errno = ECANCELED;
            return -1;
        }
        if(result > max_path)
            max_path = result;

        s[i] = g[i]->value;
    }

    return max_path;
}