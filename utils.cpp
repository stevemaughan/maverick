//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"


unsigned long time_now()
{
#if defined(__arm__) || defined(__linux__) || defined(__MINGW32__)
    return GetTickCount();

#elif defined _WIN32 && !_WIN64
    return GetTickCount();

#else
	return GetTickCount64();
#endif
}

int index_of(char *substr, char *s)
{
    int i, w;

    w = word_count(s);

    if (w == 0)
        return -1;

    i = 0;
    do {
        if (!strcmp(word_index(i,s),substr))
            return i;
        i++;
    } while (i<w);
    return -1;
}

int number_index(int index, char *s)
{
    char *str;
    int d, r, i;

    str = word_index(index,s);
    d = 1;
    r = 0;
    i = (int)strlen(str)-1;
    while (i >= 0) {
        r += d * (str[i] - '0');
        d *= 10;
        i--;
    }
    return r;
}

char *word_index(int index, char *s)
{
    static char str[UCI_BUFFER_SIZE];
    int j, n;
    size_t l, i;

    if (strlen(s)==0)
    {
        return 0;
    }
    n = 0;
    j = 0;
    l = strlen(s);
    for (i = 0; ((n <= index) && (i < l)); i++)
    {
        if (i==0)
        {
            while ((s[i]==' ') && (s[i] != '\n')) i++;
        }
        if ((n == index) && (s[i]!=' '))
        {
            str[j] = s[i];
            j++;
        }
        if (s[i]==' ')
        {
            n++;
            if ((i > 0) && (s[i-1] == ' '))
            {
                n--;
            }
        }
    }
    if (s[strlen(s)-1] == ' ') n--;
    str[j] = 0;
    return str;
}

int word_count(char *s)
{
    int n;
    size_t l, i;

    if (strlen(s)==0)
    {
        return 0;
    }
    n = 1;
    l = strlen(s);
    for (i=0; i<l; i++)
    {
        if (i==0)
        {
            while ((s[i]==' ') && (s[i] != '\n')) i++;
        }
        if (s[i]==' ')
        {
            n++;
            if ((i>0) && (s[i-1]==' '))
            {
                n--;
            }
        }
    }
    if (s[strlen(s)-1]==' ') n--;
    return n;
}

char *leftstr(char *s, int index)
{
    static char ls[1024];
    int i, c;

    strcpy(ls, "");
    c = word_count(s);
    if (c - 1 >= index) {
        strcpy(ls, word_index(index, s));
        for (i = index + 1; i < c; i++) {
            strcat(ls, " ");
            strcat(ls, word_index(i, s));
        }
    }
    strcat(ls, "\n");
    return ls;
}

t_hash rand64()
{
    t_hash i64;

    i64 = rand();
    i64 = (i64 << 24) ^ rand();
    i64 = (i64 << 24) ^ rand();

    return i64;
}

void qsort_moves(struct t_move_list *move_list, int first, int last)
{
    int						low, high;
    signed long long		midval;
    struct t_move_record	*temp_move;
    signed long long		temp_value;

    low = first;
    high = last;

    assert(first <= last);
    assert(first >= 0);
    assert(last < 256);

    midval = (move_list->value[low] + move_list->value[high]) >> 1;

    while (low < high) {

        while ((move_list->value[low] > midval) && (low < last))
            low++;
        while ((move_list->value[high] < midval) && (high > first))
            high--;

        if (low<=high) {
            temp_move = move_list->move[low];
            move_list->move[low] = move_list->move[high];
            move_list->move[high] = temp_move;

            temp_value = move_list->value[low];
            move_list->value[low] = move_list->value[high];
            move_list->value[high] = temp_value;

            low++;
            high--;
        }
    }

    if (first < high) qsort_moves(move_list, first, high);
    if (low < last) qsort_moves(move_list, low, last);
}
