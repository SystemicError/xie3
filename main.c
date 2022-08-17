/* main.c */
/* Started 2022/08/15 by Tevis Tsai */
/* Small program to help write pinyin to a file (so it can be piped to xclip or wl-clip), or just to write it */

#include<ncurses.h>
#include<stdlib.h>
#include<string.h>
#include<locale.h>

#define TRANSLATION_LENGTH 4096

typedef struct hanzi
{
    char simplified[8];
    char traditional[8];
    char pinyin[8];
    char translation[128];
} hanzi;

void draw_text(char* text)
{
    /* draw the "settled" text on screen */
    printw("%s", text);
    return;
}

char* get_match(char* pinyin, int pinyin_length, hanzi* dictionary, int num_entries, int hint_select, int trad)
{
    /* find the nth match of our current pinyin in the dictionary */
    int i, j, match, num_matches;

    if(pinyin_length == 0)
        return 0;

    num_matches = 0;
    for(i = 0 ; i < num_entries ; i ++)
    {
        match = 1;
        for(j = 0 ; j < pinyin_length ; j++)
            if(pinyin[j] != dictionary[i].pinyin[j+1])
            match = 0;
        if(match)
        {
            if(num_matches == hint_select)
                return trad?dictionary[i].traditional:dictionary[i].simplified;
            num_matches++;
        }
    }
    printf("Failed to select match.");
    return 0;
}

void draw_hints(char* pinyin, int pinyin_length, hanzi* dictionary, int num_entries, int *hint_select, int trad)
{
    /* draw the candidate hanzi and translations matching the thus-far entered pinyin */
    int i, j, match, num_matches, translation_index;

    if(pinyin_length == 0)
        return;

    printw("%s", pinyin);

    translation_index = -1;
    num_matches = 0;
    for(i = 0 ; i < num_entries ; i ++)
    {
        match = 1;
        for(j = 0 ; j < pinyin_length ; j++)
            if(pinyin[j] != dictionary[i].pinyin[j+1])
            match = 0;
        if(match)
        {
            if(num_matches == *hint_select)
            {
                attron(COLOR_PAIR(2));
                printw( "%s", trad?dictionary[i].traditional:dictionary[i].simplified);
                attroff(COLOR_PAIR(2));
                translation_index = i;
            }
        else
            {
                attron(COLOR_PAIR(1));
                printw( "%s", trad?dictionary[i].traditional:dictionary[i].simplified);
                attroff(COLOR_PAIR(1));
            }
            num_matches++;
        }
    }

    if(*hint_select >= num_matches)
        *hint_select = -1;

    if(translation_index != -1)
    {
       // printw("%s", dictionary[translation_index].translation + 1);
        printw("%s", dictionary[translation_index].translation);
       // printw("%s", dictionary[translation_index].traditional);
    }

    return;
}

int count_entries(char *path)
{
    FILE *fp;
    int count;
    char ch;
    fp = fopen(path, "r");
    count = 0;
    while(!feof(fp))
    {
        ch = fgetc(fp);
        if(ch == '\n')
            count++;
    }
    fclose(fp);
    return count;
}

hanzi* load_dictionary(char* path, int* num_entries)
{
    /* loads a dictionary from path and creates a table of simplified, traditional, pinyin, and translation of each character */
    hanzi *dict;
    int i, j;
    char simplified[8];
    char traditional[8];
    char pinyin[8];
    char translation[TRANSLATION_LENGTH];
    FILE *fp;

    *num_entries = count_entries(path);
    printf("Found %d entries.\n", *num_entries);

    dict = calloc(*num_entries, sizeof(hanzi));

    fp = fopen(path, "r");
    for(i = 0 ; i < *num_entries ; i++)
    {
        if(!fscanf(fp, "%s", traditional))
            exit(1);
        if(!fscanf(fp, "%s", simplified))
            exit(1);
        if(!fscanf(fp, "%s", pinyin))
            exit(1);
        //if(!fgets(translation, TRANSLATION_LENGTH, fp))
        //if(!fscanf(fp, "%s", translation))
        //    exit(1);
        j = 0;
	while((translation[j] = fgetc(fp)) != '\n')
	    j++;
	translation[j] = 0;

        strcpy(dict[i].simplified, simplified);
        strcpy(dict[i].traditional, traditional);
        strcpy(dict[i].pinyin, pinyin);
        strcpy(dict[i].translation, translation);
    }

    fclose(fp);
    return dict;
}

void destroy_dictionary(hanzi* dict)
{
    free(dict);
    return;
}

int main(int argc, char *argv[])
{
    hanzi *dictionary;
    int looping, i, num_entries;
    char pinyin[8];
    int pinyin_length;
    FILE *fp;
    char text[65536];
    char buffer[65536];
    char ch;
    char *str;
    int hint_select, trad;

    /* check args */
    if(argc < 2)
    {
        printf("Need file output arg.\n");
    exit(1);
    }
    if(argc >= 3)
        trad = 1;
    else
        trad = 0;

    /* set locale */
    setlocale(LC_ALL, "en_US.UTF-8");

    /* load dictionary */
    dictionary = load_dictionary("single_char.txt", &num_entries);

    /* open output file */

    fp = fopen(argv[1],"w");

    /* initialize text vars */

    text[0] = 0;

    /* initialize hint vars */

    pinyin_length = 0;
    for(i = 0 ; i < 8 ; i ++)
      pinyin[i] = 0;
    hint_select = 0;

    /* initialize ncurses */

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);

    looping = 1;
    while(looping)
    {
        clear();
        draw_text(text);
        draw_hints(pinyin, pinyin_length, dictionary, num_entries, &hint_select, trad);
        refresh();
    ch = getch();
        switch(ch)
    {
        case 'Q': looping = 0; break;
            case '\t':
                hint_select++;
            break;
            case '\n':
                if(hint_select >= 0) /* can't select negative index */
                {
                    str = get_match(pinyin, pinyin_length, dictionary, num_entries, hint_select, trad);
                    sprintf(buffer, "%s%s", text, str);
                    strcpy(text, buffer);
                pinyin_length = 0;  /* start over */
                    for(i = 0 ; i < 8 ; i ++)
                        pinyin[i] = 0;
                    hint_select = 0;
                }
            break;
            case '\b':
                pinyin_length = 0;  /* start over */
                for(i = 0 ; i < 8 ; i ++)
                    pinyin[i] = 0;
                hint_select = 0;
            break;
        default:
            if(pinyin_length < 8)
        {
            pinyin[pinyin_length] = ch;
            pinyin_length++;
        }
        else
                {
            pinyin_length = 0;  /* if they overflow the buffer, just start over */
                    for(i = 0 ; i < 8 ; i ++)
                        pinyin[i] = 0;
                    hint_select = 0;
                } 
            break;
    }
    }

    fprintf(fp, "%s", text);

    endwin();

    destroy_dictionary(dictionary);

    return 0;
}

