#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <dirent.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 20
#define LINES_PER_SCREEN ((SCREEN_HEIGHT - FONT_SIZE) / FONT_SIZE)
#define MAX_FILES 100
#define MAX_FILENAME 256

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void initialize() {
    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    printf("\x1b[2;0H");
    fatInitDefault();
}

int listTextFiles(char filenames[MAX_FILES][MAX_FILENAME]) {
    DIR *dir;
    struct dirent *ent;
    int count = 0;

    if ((dir = opendir("sd:/text/")) != NULL) {
        while ((ent = readdir(dir)) != NULL && count < MAX_FILES) {
            if (strstr(ent->d_name, ".txt") != NULL) {
                strncpy(filenames[count], ent->d_name, MAX_FILENAME - 1);
                filenames[count][MAX_FILENAME - 1] = '\0';
                count++;
            }
        }
        closedir(dir);
    }
    return count;
}

int selectFile(char filenames[MAX_FILES][MAX_FILENAME], int count) {
    int selection = 0;
    while (1) {
        printf("\x1b[2J");  // Clear the screen
        printf("\x1b[2;0H"); // Send cursor to line 2
        printf("Select a file to read:\n\n");
        for (int i = 0; i < count; i++) {
            if (i == selection) {
                printf("> ");
            } else {
                printf("  ");
            }
            printf("%s\n", filenames[i]);
        }
        printf("\nUse UP/DOWN to navigate, A to select\n");

        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_UP) {
            selection = (selection - 1 + count) % count;
        } else if (pressed & WPAD_BUTTON_DOWN) {
            selection = (selection + 1) % count;
        } else if (pressed & WPAD_BUTTON_A) {
            return selection;
        }

        VIDEO_WaitVSync();
    }
}

void displayFileContents(const char* filename) {
    char filepath[MAX_FILENAME + 7];  // +7 for "sd:/text/"
    snprintf(filepath, sizeof(filepath), "sd:/text/%s", filename);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filepath);
        return;
    }

    char line[256];
    int line_count = 0;

    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
        line_count++;

        if (line_count % LINES_PER_SCREEN == 0) {
            printf("\nPress A to continue...");
            while(1) {
                WPAD_ScanPads();
                if (WPAD_ButtonsDown(0)) {
                    break;
                }
            }
            printf("\n");
        }
    }

    fclose(file);
}

int main() {
    initialize();

    char filenames[MAX_FILES][MAX_FILENAME];
    int file_count = listTextFiles(filenames);

    if (file_count == 0) {
        printf("No text files found in sd:/text/\n");
        printf("Press any button to exit...\n");
        while(1) {
            WPAD_ScanPads();
            if (WPAD_ButtonsDown(0)) {
                printf("exiting...");
                exit(0);
            }
        }
    }

    int selected = selectFile(filenames, file_count);

    printf("\x1b[2J");  // Clear the screen
    displayFileContents(filenames[selected]);

    printf("\nEnd of file. Press any button to exit...\n");
    while(1) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0)) {
            printf("exiting...");
            exit(0);
        }
    }

    return 0;
}
