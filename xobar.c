/*
   XOBAR (x-oh-bar)
   lightweight i3status alternative
   ********************************
   written by 3hy (@9JH1)

*/

#include "toml.c"
#include <stdio.h>
#include <unistd.h>
// strcmp atoi round strcat

#define BUFFER_SIZE 1024
#define INT_MAX BUFFER_SIZE
#define INT_MIN BUFFER_SIZE * -1
// max command byte sizes
int strcmp(const char *str1, const char *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}
char *strcat(char *dest, const char *src)
{
    char *dest_end = dest;
    while (*dest_end != '\0')
    {
        dest_end++;
    }
    while (*src != '\0')
    {
        *dest_end++ = *src++;
    }
    *dest_end = '\0';
    return dest;
}
int stn(char *s)
{
    int sign = 1, res = 0, idx = 0;

    // Ignore leading whitespaces
    while (s[idx] == ' ')
    {
        idx++;
    }

    // Store the sign of number
    if (s[idx] == '-' || s[idx] == '+')
    {
        if (s[idx++] == '-')
        {
            sign = -1;
        }
    }

    // Construct the number digit by digit
    while (s[idx] >= '0' && s[idx] <= '9')
    {

        // Handling overflow/underflow test case
        if (res > INT_MAX / 10 || (res == INT_MAX / 10 && s[idx] - '0' > 7))
        {
            return sign == 1 ? INT_MAX : INT_MIN;
        }

        // Append current digit to the result
        res = 10 * res + (s[idx++] - '0');
    }
    return res * sign;
}
char *systemc(const char *command)
{
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        return NULL;
    }
    char *output = MALLOC(BUFFER_SIZE);
    if (output == NULL)
    {
        perror("malloc");
        pclose(fp);
        return NULL;
    }
    size_t length = 0;
    size_t buffer_size = BUFFER_SIZE;
    while (fgets(output + length, (int)(buffer_size - length), fp) != NULL)
    {
        length = strlen(output);
        if (length >= buffer_size - 1)
        {
            // Resize the buffer if needed
            buffer_size *= 2;
            char *new_output = realloc(output, buffer_size);
            if (new_output == NULL)
            {
                perror("realloc");
                FREE(output);
                pclose(fp);
                return NULL;
            }
            output = new_output;
        }
    }
    pclose(fp);
    if (length < buffer_size - 1)
    {
        output[length] = '\0';
    }
    else
    {
        output[buffer_size - 1] = '\0';
    }
    return output;
}
int gbs()
{
    char *com = "tput cols";
    int bar = stn(systemc(com));
    return bar;
}
void htr(const char *hex, int *r, int *g, int *b)
{
    // Ensure the hex string is valid
    if (hex[0] != '#' || strlen(hex) != 7)
    {
        fprintf(stderr, "Invalid hex color code.\n");
        exit(1);
    }
    // Convert the hex string to RGB values
    *r = (int)strtol(hex + 1, NULL, 16) >> 16;
    *g = ((int)strtol(hex + 1, NULL, 16) >> 8) & 0xFF;
    *b = (int)strtol(hex + 1, NULL, 16) & 0xFF;
}
void printc(const char *fg_hex, const char *bg_hex, const char *text)
{
    int foreground_r, foreground_g, foreground_b;
    int background_r, background_g, background_b;
    char escape_seq[128] = "\033["; // Buffer to build the escape sequence
    int first = 1;

    // Initialize escape sequence buffer
    escape_seq[0] = '\033';
    escape_seq[1] = '[';
    escape_seq[2] = '\0';

    // Convert and apply foreground color if provided
    if (fg_hex != NULL || fg_hex != "")
    {
        htr(fg_hex, &foreground_r, &foreground_g, &foreground_b);
        if (!first)
        {
            strcat(escape_seq, ";");
        }
        strcat(escape_seq, "38;2;");
        char temp[32];
        snprintf(temp, sizeof(temp), "%d;%d;%d", foreground_r, foreground_g, foreground_b);
        strcat(escape_seq, temp);
        first = 0;
    }

    // Convert and apply background color if provided
    if (bg_hex != NULL || fg_hex != "")
    {
        if (!first)
        {
            strcat(escape_seq, ";");
        }
        strcat(escape_seq, "48;2;");
        htr(bg_hex, &background_r, &background_g, &background_b);
        char temp[32];
        snprintf(temp, sizeof(temp), "%d;%d;%d", background_r, background_g, background_b);
        strcat(escape_seq, temp);
    }

    // Finalize the escape sequence and print
    if (strlen(escape_seq) > 2)
    {
        strcat(escape_seq, "m");
        printf("%s%s\033[0m", escape_seq, text);
    }
    else
    {
        // No color escape sequence, just print text normally
        printf("%s\033[0m", text);
    }
}
static void vout(const char *msg, const char *msg1, int verbose, int error)
{
    if (verbose == 1)
    {
        printf("%s %s");
    }
    if (error == 1)
    {
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    // base vars
    FILE *fp;
    char errbuf[256];
    char *configFile = NULL,
         *bar = "";
    int verbose = 0,
        removeSize = 0,
        maxModuleTitleSize = 30,
        recurse = 0,
        recurseTimeout = 5;

    // condition vars on arguments
    for (int a = 1; a < argc; a++)
    {
        // make it easier to understand
        char *argument = argv[a];
        char *nextArgument = argv[a + 1];
        if (strcmp(argument, "-v") == 0)
        { // set verbose to one
            verbose = 1;
        }
        else if (strcmp(argument, "-c") == 0)
        { // config file argument "-c"
            configFile = nextArgument;
        }
        else if (strcmp(argument, "-r") == 0)
        { // recurse mode on
            recurse = 1;
            recurseTimeout = atoi(nextArgument);
        }
    }

    // load the config file
    if (configFile == NULL)
    {
        vout("ERROR: ", "file not found", verbose, 1);
    }
    else
    {
        fp = fopen(configFile, "r");
        if (!fp)
        {
            vout("ERROR: ", "cannot open config file", verbose, 1);
        }
    }

    // load the config toml
    toml_table_t *config = toml_parse_file(fp, errbuf, sizeof(errbuf));
    if (!config)
    {
        vout("ERROR: cannot parse config file", errbuf, verbose, 0);
    }

    // load the settings
    toml_table_t *settings = toml_table_in(config, "settings");
    if (!settings)
    {
        vout("ERROR: ", "[settings] module not found", verbose, 1);
    }
    else
    {
        vout("INFO: ", "[settings] module loaded", verbose, 0);
    }

    // assign the settings variables
    int settingsSpacerWidths = toml_int_in(settings, "spacer-width").u.i,
        settingsPaddingInner = toml_int_in(settings, "padding-inner").u.i,
        settingsPaddingOuter = toml_int_in(settings, "padding-outer").u.i,
        barSizeHalf = (gbs() / 2) - settingsPaddingOuter * 2;
    char *settingsBackground = toml_string_in(settings, "background").u.s,
         *settingsForeground = toml_string_in(settings, "foreground").u.s,
         *settingsSpacerChar = toml_string_in(settings, "spacer-char").u.s,
         *settingsPaddingForeground = toml_string_in(settings, "padding-foreground").u.s,
         *settingsPaddingBackground = toml_string_in(settings, "padding-background").u.s;
    // swap out null items
    if (settingsSpacerChar == NULL)
    {
        settingsSpacerChar = " ";
    }
    // add the very first padding char of padding outer
    for (int tmp_paddingInner = 0; tmp_paddingInner < settingsPaddingInner; tmp_paddingInner++)
    {
        printc(settingsForeground, settingsBackground, settingsSpacerChar);
    }
mainLoop:
    // content

    // EOF operations
    for (int tmp_paddingInner = 0; tmp_paddingInner < settingsPaddingInner; tmp_paddingInner++)
    {
        printc(settingsForeground, settingsBackground, settingsSpacerChar);
    }
    if (recurse == 1)
    {
        sleep(recurseTimeout);
        goto mainLoop;
    }
    return 0;
}