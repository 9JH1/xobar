#include "toml.c"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <math.h>

#define BUFFER_SIZE 1024

int getBarSize()
{
    int cols = 0;
    struct winsize window;
    ioctl(0, TIOCGWINSZ, &window);
    cols = window.ws_col;
    return cols;
}

// Function to execute a command and return its output
char *execute_command(const char *command)
{
    // Open a pipe to read the command's output
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        return NULL;
    }

    // Allocate a buffer to store the command output
    char *output = MALLOC(BUFFER_SIZE);
    if (output == NULL)
    {
        perror("malloc");
        pclose(fp);
        return NULL;
    }

    // Read the output into the buffer
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

    // Close the pipe
    pclose(fp);

    // Null-terminate the string
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

// Function to convert a hex color code to RGB values
void hex_to_rgb(const char *hex, int *r, int *g, int *b)
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

// Updated function to use hex color codes
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
        hex_to_rgb(fg_hex, &foreground_r, &foreground_g, &foreground_b);
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
        hex_to_rgb(bg_hex, &background_r, &background_g, &background_b);
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

static void error(const char *msg, const char *msg1, int verbose)
{
    if (verbose == 1)
    {
        fprintf(stderr, "ERROR: %s%s\n", msg, msg1 ? msg1 : "");
    }
    exit(1);
}

int main(int argc, char *argv[])
{
    FILE *fp;
    char errbuf[200];
    char *config_file = NULL;
    int verbose = 0;
    int removeSize = 0;
    int moduleAmount = 10;
    int barPaddingSizeFull = 0;
    int maxModuleTitleSize = 30;
    int barSizeHalf = round(getBarSize() / 2);
    int recurse = 0;
    int recurseTimeout = 5;

    // skip first arg ( file )
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-c") == 0)
        {

            config_file = argv[i + 1];
            if (verbose == 1)
            {
                printf("INFO: set config file %s\n", config_file);
            }
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            verbose = 1;
            printf("INFO: set verbose on\n");
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            moduleAmount = atoi(argv[i + 1]);
            if (verbose == 1)
            {
                printf("INFO: set module amount to %d\n", moduleAmount);
            }
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            maxModuleTitleSize = atoi(argv[i + 1]);
            if (verbose == 1)
            {
                printf("INFO: set max module title size %d\n", maxModuleTitleSize);
            }
        }
        else if (strcmp(argv[i], "-r") == 0)
        {

            recurseTimeout = atoi(argv[i + 1]);
            if (verbose == 1)
            {
                printf("INFO: set max module title size %d\n", recurseTimeout);
            }
        }
    }

    char moduleList[9][1] = {};

    // check if the config has been entered
    if (config_file == NULL)
    {
        // config has not been found
        // default to file
        fp = fopen("~/xobar.toml", "r");
    }
    else
    {
        fp = fopen(config_file, "r");
    }

    // check if for file status
    if (!fp)
    {
        // file does not exist or some error occurred
        error("cannot open config file - ", strerror(errno), verbose);
    }

    // load toml data from file
    toml_table_t *config = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!config)
    {
        error("cannot parse - ", errbuf, verbose);
    }
    else
    {
        if (verbose == 1)
        {
            printf("INFO: loaded config file %s\n", config_file);
        }
    }

    // load the settings config
    toml_table_t *settings = toml_table_in(config, "settings");
    if (!settings)
    {
        if (verbose == 1)
        {
            error("[settings] not found", "", verbose);
        }
    }
    if (verbose == 1)
    {
        printf("INFO: loaded settings table %s\n");
    }

    // get background color
    toml_datum_t background = toml_string_in(settings, "background");
    if (!background.ok)
    {
        if (verbose == 1)
        {
            printf("INFO: cannot find background in [settings]\n");
        }
    }

    // get foreground color
    toml_datum_t foreground = toml_string_in(settings, "foreground");
    if (!foreground.ok)
    {
        if (verbose == 1)
        {
            printf("INFO: cannot find foreground in [settings]\n");
        }
    }

    // get padding
    toml_datum_t settingsPadding = toml_int_in(settings, "padding");
    toml_datum_t settingsPaddingInner = toml_int_in(settings, "padding-inner");
    toml_datum_t settingsPaddingChar = toml_string_in(settings, "spacer");
    toml_datum_t settingsSpacerBackground = toml_string_in(settings, "spacer-background");
    toml_datum_t settingsSpacerForeground = toml_string_in(settings, "spacer-foreground");

    // time to get the sides :/
    // padding for first side
    for (int i = 0; i < settingsPadding.u.i; i++)
    {
        printc(foreground.u.s, background.u.s, settingsPaddingChar.u.s);
        removeSize += strlen(settingsPaddingChar.u.s);
    }
    // left
    toml_table_t *left = toml_table_in(config, "left");
    toml_array_t *leftChildren = toml_array_in(left, "children");
    if (verbose == 1)
    {
        if (left)
        {
            printf("INFO: loaded left table\n");
            printf("INFO: read modules from left:\n");
        }
        else
        {
            printf("INFO: did not load left table\n");
        }
    }
    if (leftChildren)
    {
        for (int i = 0;; i++)
        {
            toml_datum_t leftChild = toml_string_at(leftChildren, i);
            if (!leftChild.ok)
                break;
            if (verbose == 1)
            {
                printf("    - %s\n", leftChild.u.s);
            }
            // load each of the left modules :3
            toml_table_t *leftModule = toml_table_in(config, leftChild.u.s);
            toml_datum_t leftModuleType = toml_string_in(leftModule, "type");

            toml_datum_t leftBackground = toml_string_in(left, "background");
            toml_datum_t leftForeground = toml_string_in(left, "foreground");
            toml_datum_t leftModuleBackground = toml_string_in(leftModule, "background");
            toml_datum_t leftModuleForeground = toml_string_in(leftModule, "foreground");
            toml_datum_t leftModuleContent;
            char *leftModuleContentString;
            if (strcmp(leftModuleType.u.s, "text") == 0)
            {
                // module is text based
                leftModuleContent = toml_string_in(leftModule, "content");
                leftModuleContentString = leftModuleContent.u.s;
            }
            else
            {
                // module is something else ( script )
                leftModuleContent = toml_string_in(leftModule, "exec");
                leftModuleContentString = execute_command(leftModuleContent.u.s);
            }
            // print out the content if it is text
            if (!leftModuleBackground.ok)
            {
                if (!leftBackground.ok)
                {
                    leftModuleBackground = background;
                }
                else
                {
                    leftModuleBackground = leftBackground;
                }
            }
            if (!leftModuleForeground.ok)
            {
                if (!leftForeground.ok)
                {
                    leftModuleForeground = foreground;
                }
                else
                {
                    leftModuleForeground = leftForeground;
                }
            }

            printc(leftModuleForeground.u.s, leftModuleBackground.u.s, leftModuleContentString);
            removeSize += strlen(leftModuleContentString);
            toml_datum_t leftModulePaddingSpacer = toml_string_in(left, "spacer");

            toml_datum_t leftPaddingBackground = toml_string_in(left, "padding-background");
            toml_datum_t leftPaddingForeground = toml_string_in(left, "padding-foreground");
            if (!leftPaddingBackground.ok)
            {
                leftPaddingBackground = leftModuleBackground;
            }
            if (!leftPaddingForeground.ok)
            {
                leftPaddingForeground = leftModuleForeground;
            }

            // print the padding
            for (int ii = 0; ii < settingsPaddingInner.u.i; ii++)
            {
                printc(leftPaddingForeground.u.s, leftPaddingBackground.u.s, leftModulePaddingSpacer.u.s);
                removeSize += strlen(leftModulePaddingSpacer.u.s);
            }
        }
        if (verbose == 1)
        {
            printf("\n");
        }
        /*
        for (int i = 0; i < getBarSize() / 2 - removeSize; i++)
        {
            printc(foreground.u.s, background.u.s, settingsPaddingChar.u.s);
        }*/
    }

    toml_table_t *center = toml_table_in(config, "center");
    if (center)
    {
        int removeSizeLocalCenter = 0;
        int childCountCenter = 0;
        toml_array_t *centerChildren = toml_array_in(center, "children");
        if (verbose == 1)
        {
            if (center)
            {
                printf("INFO: loaded center table\n");
                printf("INFO: read modules from center:\n");
            }
            else
            {
                printf("INFO: did not load center table\n");
            }
        }
        if (centerChildren)
        {
            for (int i = 0;; i++)
            {
                toml_datum_t centerChild = toml_string_at(centerChildren, i);
                if (!centerChild.ok)
                    break;
                if (verbose == 1)
                {
                    printf("    - %s\n", centerChild.u.s);
                }

                // load each of the center modules :3
                toml_table_t *centerModule = toml_table_in(config, centerChild.u.s);
                toml_datum_t centerModuleType = toml_string_in(centerModule, "type");
                toml_datum_t centerModuleContent;
                childCountCenter++;
                char *centerModuleContentString;
                for (int ii = 0; ii < settingsPaddingInner.u.i; ii++)
                {
                    removeSizeLocalCenter += strlen(settingsPaddingChar.u.s);
                }
                if (strcmp(centerModuleType.u.s, "text") == 0)
                {
                    // module is text based
                    centerModuleContent = toml_string_in(centerModule, "content");
                    centerModuleContentString = centerModuleContent.u.s;
                }
                else
                {
                    // module is something else ( script )
                    centerModuleContent = toml_string_in(centerModule, "exec");
                    centerModuleContentString = execute_command(centerModuleContent.u.s);
                }
                toml_datum_t centerModulePaddingSpacer = toml_string_in(center, "spacer");
                removeSizeLocalCenter += strlen(centerModuleContentString);
            }

            if (childCountCenter > 0)
            {
                removeSizeLocalCenter -= strlen(settingsPaddingChar.u.s);
                removeSizeLocalCenter -= (strlen(settingsPaddingChar.u.s) * settingsPaddingInner.u.i);
            }
            int finalBarSizeCenter = ((getBarSize() - barSizeHalf) - removeSize) - (removeSizeLocalCenter / 2);
            if (!settingsSpacerForeground.ok)
            {
                settingsSpacerForeground = foreground;
            }
            if (!settingsSpacerBackground.ok)
            {
                settingsSpacerBackground = background;
            }
            for (int i = 0; i < finalBarSizeCenter; i++)
            {
                printc(settingsSpacerForeground.u.s, settingsSpacerBackground.u.s, settingsPaddingChar.u.s);
                barPaddingSizeFull++;
            }
            for (int i = 0;; i++)
            {
                toml_datum_t centerChild = toml_string_at(centerChildren, i);
                if (!centerChild.ok)
                    break;
                if (verbose == 1)
                {
                    printf("    - %s\n", centerChild.u.s);
                }
                // load each of the center modules :3
                toml_table_t *centerModule = toml_table_in(config, centerChild.u.s);
                toml_datum_t centerModuleType = toml_string_in(centerModule, "type");

                toml_datum_t centerBackground = toml_string_in(center, "background");
                toml_datum_t centerForeground = toml_string_in(center, "foreground");
                toml_datum_t centerModuleBackground = toml_string_in(centerModule, "background");
                toml_datum_t centerModuleForeground = toml_string_in(centerModule, "foreground");
                toml_datum_t centerModuleContent;
                char *centerModuleContentString;
                if (strcmp(centerModuleType.u.s, "text") == 0)
                {
                    // module is text based
                    centerModuleContent = toml_string_in(centerModule, "content");
                    centerModuleContentString = centerModuleContent.u.s;
                }
                else
                {
                    // module is something else ( script )
                    centerModuleContent = toml_string_in(centerModule, "exec");
                    centerModuleContentString = execute_command(centerModuleContent.u.s);
                }
                // print out the content if it is text
                if (!centerModuleBackground.ok)
                {
                    if (!centerBackground.ok)
                    {
                        centerModuleBackground = background;
                    }
                    else
                    {
                        centerModuleBackground = centerBackground;
                    }
                }
                if (!centerModuleForeground.ok)
                {
                    if (!centerForeground.ok)
                    {
                        centerModuleForeground = foreground;
                    }
                    else
                    {
                        centerModuleForeground = centerForeground;
                    }
                }

                printc(centerModuleForeground.u.s, centerModuleBackground.u.s, centerModuleContentString);
                removeSize += strlen(centerModuleContentString);
                toml_datum_t centerModulePaddingSpacer = toml_string_in(center, "spacer");

                toml_datum_t centerPaddingBackground = toml_string_in(center, "padding-background");
                toml_datum_t centerPaddingForeground = toml_string_in(center, "padding-foreground");
                if (!centerPaddingBackground.ok)
                {
                    centerPaddingBackground = centerModuleBackground;
                }
                if (!centerPaddingForeground.ok)
                {
                    centerPaddingForeground = centerModuleForeground;
                }
                if (!centerModulePaddingSpacer.ok)
                {
                    centerModulePaddingSpacer = settingsPaddingChar;
                }
                // print the padding
                for (int ii = 0; ii < settingsPaddingInner.u.i; ii++)
                {
                    printc(centerPaddingForeground.u.s, centerPaddingBackground.u.s, centerModulePaddingSpacer.u.s);
                    removeSize += strlen(centerModulePaddingSpacer.u.s);
                }
            }
        }
        if (verbose == 1)
        {
            printf("\n");
        }
    }
    // right
    // can be dine (fullSize - removeSize )-strlen(rightLocaltext)
    toml_table_t *right = toml_table_in(config, "right");
    if (right)
    {

        int removeSizeLocalRight = 0;
        int childCountRight = 0;
        toml_array_t *rightChildren = toml_array_in(right, "children");
        if (verbose == 1)
        {
            if (right)
            {
                printf("INFO: loaded right table\n");
                printf("INFO: read modules from right:\n");
            }
            else
            {
                printf("INFO: did not load right table\n");
            }
        }
        if (rightChildren)
        {
            for (int i = 0;; i++)
            {
                toml_datum_t rightChild = toml_string_at(rightChildren, i);
                if (!rightChild.ok)
                    break;

                // load each of the right modules :33
                toml_table_t *rightModule = toml_table_in(config, rightChild.u.s);
                toml_datum_t rightModuleType = toml_string_in(rightModule, "type");
                toml_datum_t rightModuleContent;

                childCountRight++;
                char *rightModuleContentString;

                for (int ii = 0; ii < settingsPaddingInner.u.i; ii++)
                {
                    removeSizeLocalRight += strlen(settingsPaddingChar.u.s);
                }
                if (strcmp(rightModuleType.u.s, "text") == 0)
                {
                    // module is text based
                    rightModuleContent = toml_string_in(rightModule, "content");
                    rightModuleContentString = rightModuleContent.u.s;
                }
                else
                {
                    // module is something else ( script )
                    rightModuleContent = toml_string_in(rightModule, "exec");
                    rightModuleContentString = execute_command(rightModuleContent.u.s);
                }
                if (verbose == 1)
                {
                    printf("    - %s\n", rightChild.u.s);
                }
                removeSizeLocalRight += strlen(rightModuleContentString);
            }
            // the minus one is for the pading char we are gonna print at the end of the file
            for (int i = 0; i < (getBarSize()) - barPaddingSizeFull - removeSize - settingsPadding.u.i - removeSizeLocalRight; i++)
            {
                printc(settingsSpacerForeground.u.s, settingsSpacerBackground.u.s, settingsPaddingChar.u.s);
            }
            for (int i = 0;; i++)
            {
                toml_datum_t rightChild = toml_string_at(rightChildren, i);
                if (!rightChild.ok)
                    break;
                toml_table_t *rightModule = toml_table_in(config, rightChild.u.s);
                toml_datum_t rightModuleType = toml_string_in(rightModule, "type");
                toml_datum_t rightBackground = toml_string_in(right, "background");
                toml_datum_t rightForeground = toml_string_in(right, "foreground");
                toml_datum_t rightModuleBackground = toml_string_in(rightModule, "background");
                toml_datum_t rightModuleForeground = toml_string_in(rightModule, "foreground");
                // do the background checks
                // fall back to the parent if there is no background

                // print the actual item
                // more vars
                toml_datum_t rightModuleContent;
                char *rightModuleContentString;

                for (int ii = 0; ii < settingsPaddingInner.u.i; ii++)
                {
                    // we dont need to worry about counting anything in this loop
                    // because we have nothing else to use the spacing for
                }
                if (strcmp(rightModuleType.u.s, "text") == 0)
                {
                    rightModuleContent = toml_string_in(rightModule, "content");
                    rightModuleContentString = rightModuleContent.u.s;
                }
                else
                {
                    rightModuleContent = toml_string_in(rightModule, "exec");
                    rightModuleContentString = execute_command(rightModuleContent.u.s);
                    // still no scripts but ill come back to this soon
                }

                if (!rightModuleBackground.ok)
                {
                    if (!rightBackground.ok)
                    {
                        rightModuleBackground = background;
                    }
                    else
                    {
                        rightModuleBackground = rightBackground;
                    }
                }
                if (!rightModuleForeground.ok)
                {
                    if (!rightForeground.ok)
                    {
                        rightModuleForeground = foreground;
                    }
                    else
                    {
                        rightModuleForeground = rightForeground;
                    }
                }
                for (int ii = 0; ii < settingsPaddingInner.u.i; ii++)
                {
                    printc(settingsSpacerForeground.u.s, settingsSpacerBackground.u.s, settingsPaddingChar.u.s);
                }
                printc(rightModuleForeground.u.s, rightModuleBackground.u.s, rightModuleContentString);
            }
        }
        if (verbose == 1)
        {
            printf("\n");
        }
    }
    for (int i = 0; i < settingsPadding.u.i; i++)
    {
        printc(foreground.u.s, background.u.s, settingsPaddingChar.u.s);
        removeSize += strlen(settingsPaddingChar.u.s);
    }
    toml_free(config);
    return 0;
}
