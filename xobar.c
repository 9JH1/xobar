/*
   XOBAR (x-oh-bar)
   lightweight i3status alternative
   ********************************
   written by 3hy (@9JH1)

*/

#include "toml.c"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
// no libs lol

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
    if (fg_hex == NULL && bg_hex == NULL)
    {
        printf("%s", text);
        return;
    }
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
    if (bg_hex != NULL || bg_hex != "")
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
void vout(const char *msg, const char *msg1, int vb, int error)
{
    if (vb == 1)
    {
        printf("%s %s\n", msg, msg1);
    }
    if (error == 1)
    {
        exit(1);
    }
}
void qh()
{
    printf("\n\e[?25h");
    exit(0);
}

int main(int argc, char *argv[])
{
    // hide the cursor
    printf("\e[?25l");

    // start quit listener
    signal(SIGINT, qh);
    // base vars
    FILE *fp;
    char errbuf[256];
    char *configFile = NULL,
         *bar = "";
    int removeSize = 0,
        maxModuleTitleSize = 30,
        recurse = 0,
        recurseTimeout = 5,
        verbose = 0,
        tail = 0,
        size = 0,
        resizable = 0;

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
        else if (strcmp(argument, "-c") == 0 && nextArgument != NULL)
        { // config file argument "-c"
            configFile = nextArgument;
        }
        else if (strcmp(argument, "-l") == 0 && nextArgument != NULL)
        { // recurse mode on
            recurse = 1;
            recurseTimeout = atoi(nextArgument);
        }
        else if (strcmp(argument, "-t") == 0)
        {
            tail = 1;
        }
        else if (strcmp(argument, "-s") == 0 && nextArgument != NULL)
        {
            size = atoi(nextArgument);
        }
        else if (strcmp(argument, "-r") == 0)
        {
            resizable = 1;
        }
        else
        {
            // argument not found
            vout("ERROR: invalid argument property:", argument, verbose, 0);
        }
    }

    // load the config file
    if (configFile == NULL)
    {
        vout("ERROR:", "file not found", verbose, 1);
    }
    else
    {
        fp = fopen(configFile, "r");
        if (!fp)
        {
            vout("ERROR: cannot open config file", configFile, verbose, 1);
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
        vout("ERROR: [settings] module not found", "", verbose, 0);
    else
        vout("INFO: [settings] module loaded", "", verbose, 0);

    // assign the settings variables
    int settingsSpacerWidths = toml_int_in(settings, "spacer-width").u.i,
        settingsPaddingInner = toml_int_in(settings, "padding-inner").u.i,
        settingsPaddingOuter = toml_int_in(settings, "padding-outer").u.i,
        barSize = 0;
    if (size >= 0)
    {
        barSize = size;
    }
    else
    {
        barSize = gbs();
    }
    char *currentModule = "",
         *settingsBackground = toml_string_in(settings, "background").u.s,
         *settingsForeground = toml_string_in(settings, "foreground").u.s,
         *settingsSpacerChar = toml_string_in(settings, "spacer").u.s,
         *settingsPaddingForeground = toml_string_in(settings, "padding-foreground").u.s,
         *settingsPaddingBackground = toml_string_in(settings, "padding-background").u.s,
         *settingsPaddingSpacerChar = toml_string_in(settings, "void-spacer").u.s,
         *settingsPaddingSpacerCharForeground = toml_string_in(settings, "void-foreground").u.s,
         *settingsPaddingSpacerCharBackground = toml_string_in(settings, "void-background").u.s;
    // can add a void spacer option but ill do it later ;3
    if (settingsPaddingBackground == NULL)
    {
        settingsPaddingBackground = settingsBackground;
    }
    if (settingsPaddingForeground == NULL)
    {
        settingsPaddingForeground = settingsForeground;
    }
    if (settingsSpacerChar == NULL)
    {
        settingsSpacerChar = " ";
    }
    if (settingsPaddingSpacerChar == NULL)
    {
        settingsPaddingSpacerChar = settingsSpacerChar;
    }
    if (settingsPaddingSpacerCharBackground == NULL)
    {
        if (settingsPaddingBackground == NULL)
        {
            settingsPaddingSpacerCharBackground = settingsBackground;
        }
        else
        {
            settingsPaddingSpacerCharBackground = settingsPaddingBackground;
        }
    }
    if (settingsPaddingSpacerCharForeground == NULL)
    {
        if (settingsPaddingForeground == NULL)
        {
            settingsPaddingSpacerCharForeground = settingsForeground;
        }
        else
        {
            settingsPaddingSpacerCharForeground = settingsPaddingForeground;
        }
    }

    // swap out null items

    // get the module blocks

renderBar:
    // add the very first padding char of padding outer

// content
renderContent:
    if (barSize != gbs() && resizable == 1)
    {
        if (tail == 0)
        {
            system("clear");
        }
        if (size >= 0)
        {
            barSize = gbs();
        }
    }
    for (int itModule = 0; itModule < 3; itModule++) // 0=left, 1=center, 2=right
    {
        int currentModuleRemoveSizeLocal = 0, nextModuleRemoveSizeLocal = 0;
        if (itModule == 0)
        {
            currentModule = "right";
        }
        else if (itModule == 1)
        {
            currentModule = "center";
        }
        else
        {
            currentModule = "left";
        }
        vout("INFO: loaded required module:", currentModule, verbose, 0);
        // for MODULE
        toml_table_t *module = toml_table_in(config, currentModule);

        char *moduleBackground = toml_string_in(module, "background").u.s,
             *moduleForeground = toml_string_in(module, "foreground").u.s,
             *moduleSpacerChar = toml_string_in(module, "spacer").u.s,
             *modulePaddingForeground = toml_string_in(module, "padding-foreground").u.s,
             *modulePaddingBackground = toml_string_in(module, "padding-background").u.s;

        toml_array_t *moduleChildren /*plural*/ = toml_array_in(module, "children");
        for (int tmp_moduleLoop = 0;; tmp_moduleLoop++)
        {
            if (!moduleChildren)
                vout("FATAL_ERROR: required module not found:", currentModule, verbose, 1);

            // for SUBMODULES
            toml_datum_t tmp_moduleChild = toml_string_at(moduleChildren, tmp_moduleLoop);
            if (!tmp_moduleChild.ok)
                // if the module is not found then exit script
                break;

            vout("INFO: loaded submodule:", tmp_moduleChild.u.s, verbose, 0);

            toml_table_t *moduleChild = toml_table_in(config, tmp_moduleChild.u.s);
            if (!moduleChild)
                // if the module is not found then exit script
                vout("FATAL_ERROR: required module not found:", currentModule, verbose, 1);

            // define variables
            char
                *moduleChildType = toml_string_in(moduleChild, "type").u.s,
                *moduleChildContent;

            // get the content of the module
            if (strcmp(moduleChildType, "text") == 0)
            {
                moduleChildContent = toml_string_in(moduleChild, "content").u.s;
            }
            else
            {
                moduleChildContent = systemc(toml_string_in(moduleChild, "exec").u.s);
            }
            int childModuleLeftPadding, childModuleRightPadding;
            if (itModule == 2)
            { // module is left
                childModuleLeftPadding = 0;
                childModuleRightPadding = settingsPaddingInner;
            }
            else if (itModule == 1)
            {

                childModuleLeftPadding = settingsPaddingInner / 2;
                childModuleRightPadding = settingsPaddingInner / 2;
            }
            else
            {

                childModuleLeftPadding = settingsPaddingInner;
                childModuleRightPadding = 0;
            }
            char *spacerChar;
            if (moduleSpacerChar == NULL)
            {
                spacerChar = settingsSpacerChar;
            }
            else
            {
                spacerChar = moduleSpacerChar;
            }

            for (int i = 0; i < childModuleLeftPadding; i++)
            {
                currentModuleRemoveSizeLocal += strlen(spacerChar);
            }
            currentModuleRemoveSizeLocal += strlen(moduleChildContent);
            // right padding
            for (int i = 0; i < childModuleRightPadding; i++)
            {

                currentModuleRemoveSizeLocal += strlen(spacerChar);
            }
        }
        if (itModule == 0)
        {
            // loading the rightmost module print out filler bar text
            for (int i = 0; i < barSize - currentModuleRemoveSizeLocal - settingsPaddingOuter; i++)
            {
                printc(settingsPaddingSpacerCharForeground, settingsPaddingSpacerCharBackground, settingsPaddingSpacerChar);
            }
        }
        else if (itModule == 1)
        {
            // loading the center module

            printf("\r");
            for (int i = 0; i < barSize / 2 - currentModuleRemoveSizeLocal / 2; i++)
            {
                printc(settingsPaddingSpacerCharForeground, settingsPaddingSpacerCharBackground, settingsPaddingSpacerChar);
            }
        }
        else
        {
            printf("\r");
            for (int tmp_paddingInner = 0; tmp_paddingInner < settingsPaddingOuter; tmp_paddingInner++)
            {
                printc(settingsForeground, settingsBackground, settingsSpacerChar);
            }
        }
        for (int moduleLoop = 0;; moduleLoop++)
        {
            if (!moduleChildren)
                vout("FATAL_ERROR: required module not found:", currentModule, verbose, 1);

            // for SUBMODULES
            toml_datum_t tmp_moduleChild = toml_string_at(moduleChildren, moduleLoop);
            if (!tmp_moduleChild.ok)
                // if the module is not found then exit script
                break;

            vout("INFO: loaded submodule:", tmp_moduleChild.u.s, verbose, 0);

            toml_table_t *moduleChild = toml_table_in(config, tmp_moduleChild.u.s);
            if (!moduleChild)
                // if the module is not found then exit script
                vout("FATAL_ERROR: required module not found:", currentModule, verbose, 1);

            // define variables
            char *moduleChildBackground = toml_string_in(moduleChild, "background").u.s,
                 *moduleChildForeground = toml_string_in(moduleChild, "foreground").u.s,
                 *moduleChildType = toml_string_in(moduleChild, "type").u.s,
                 *moduleChildPre = toml_string_in(moduleChild, "prefix").u.s,
                 *moduleChildSuf = toml_string_in(moduleChild, "suffix").u.s,
                 *moduleChildContent;

            // get the content of the module
            if (strcmp(moduleChildType, "text") == 0)
            {
                moduleChildContent = toml_string_in(moduleChild, "content").u.s;
            }
            else
            {
                moduleChildContent = systemc(toml_string_in(moduleChild, "exec").u.s);
            }

            // final colors
            char *background, *foreground;

            // figure out the background
            if (moduleChildBackground == NULL)
            {
                if (moduleBackground == NULL)
                {
                    background = settingsBackground;
                }
                else
                {
                    background = moduleBackground;
                }
            }
            else
            {
                background = moduleChildBackground;
            }
            // figure out the background
            if (moduleChildForeground == NULL)
            {
                if (moduleForeground == NULL)
                {
                    foreground = settingsForeground;
                }
                else
                {
                    foreground = moduleForeground;
                }
            }
            else
            {
                foreground = moduleChildForeground;
            }

            char *paddingBackground, *paddingForeground;
            // same for the padding colors
            if (modulePaddingBackground == NULL)
            {
                if (settingsPaddingBackground == NULL)
                {
                    paddingBackground = background;
                }
                else
                {
                    paddingBackground = settingsPaddingBackground;
                }
            }
            else
            {
                paddingBackground = modulePaddingBackground;
            }
            if (modulePaddingForeground == NULL)
            {
                if (settingsPaddingForeground == NULL)
                {
                    paddingForeground = foreground;
                }
                else
                {
                    paddingForeground = settingsPaddingForeground;
                }
            }
            else
            {
                paddingForeground = modulePaddingForeground;
            }

            // and the same for the spacer char
            char *spacerChar;
            if (moduleSpacerChar == NULL)
            {
                spacerChar = settingsSpacerChar;
            }
            else
            {
                spacerChar = moduleSpacerChar;
            }
            // left padding
            int childModuleLeftPadding,
                childModuleRightPadding;
            if (itModule == 2)
            { // module is left
                childModuleLeftPadding = 0;
                childModuleRightPadding = settingsPaddingInner;
            }
            else if (itModule == 1)
            {

                childModuleLeftPadding = settingsPaddingInner / 2;
                childModuleRightPadding = settingsPaddingInner / 2;
            }
            else
            {

                childModuleLeftPadding = settingsPaddingInner;
                childModuleRightPadding = 0;
            }
            for (int i = 0; i < childModuleLeftPadding; i++)
            {
                printc(paddingForeground, paddingBackground, spacerChar);
            }
            printc(foreground, background, moduleChildContent);
            // right padding
            for (int i = 0; i < childModuleRightPadding; i++)
            {
                printc(paddingForeground, paddingBackground, spacerChar);
            }

            if (itModule == 0)
            {
                for (int tmp_paddingInner = 0; tmp_paddingInner < settingsPaddingOuter; tmp_paddingInner++)
                {
                    printc(settingsPaddingForeground, settingsPaddingBackground, settingsSpacerChar);
                }
            }
        }
    }
    fflush(stdout);
    if (recurse == 1)
    {
        sleep(recurseTimeout);
        if (tail == 0)
        {
            printf("\r");
        }
        else
        {
            printf("\n");
        }
        goto renderBar;
    }
    qh();
    return 0;
}
