#include <ncurses.h>   // Include ncurses library for creating text-based UI
#include <stdio.h>      // Standard I/O library for file handling and printing
#include <dirent.h>     // Library for directory operations
#include <ctype.h>      // Character type library to check if a character is a digit
#include <string.h>     // String manipulation library

void init_ncurses() {
    initscr();                    // Start ncurses mode to take over terminal screen
    cbreak();                     // Disable line buffering; input is immediately available  
    noecho();                     // Do not display typed characters on screen  
    curs_set(FALSE);              // Hide the cursor to keep the screen clean  
    timeout(1000);                // Set input timeout to 1000ms (1 second) for screen refresh rate

    // colors 
    if (has_colors()) {           // Check if terminal supports color
        start_color();            // Enable color functionality in ncurses
        init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Pair 1: Green text on black background
        init_pair(2, COLOR_MAGENTA, COLOR_BLACK); // Pair 2: Magenta (Purple) text on black background
        init_pair(3, COLOR_RED, COLOR_BLACK);     // Pair 3: Red text on black background
    }
}  

void close_ncurses() {
    endwin();                       // Ends ncurses mode, restores normal terminal behavior
}

// Function to read memory usage from /proc/meminfo
void get_memory_usage(unsigned long *total, unsigned long *available) {
    FILE *fp = fopen("/proc/meminfo", "r");  // Open /proc/meminfo in read mode
    if (!fp) return;                         // If the file can't be opened, return early

    char label[32];         // Temporary storage for the label (like "MemTotal:")
    unsigned long value;    // Temporary storage for the memory value

    // Loop through each line until EOF
    while (fscanf(fp, "%s %lu kB", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) *total = value;         // If label is "MemTotal:", set *total
        if (strcmp(label, "MemAvailable:") == 0) *available = value; // If label is "MemAvailable:", set *available
    }
    fclose(fp);  // Close the file after reading to free resources
}

// Function to calculate CPU usage from /proc/stat
float get_cpu_usage() {
    // Open the file that holds cpu statistics
    FILE *fp = fopen("/proc/stat", "r"); // Open /proc/stat in read mode
    if (!fp) return -1;              // If the file can't be opened, return -1 as an error

    // Declare variables to store CPU time values
    unsigned long user = 0, nice = 0, system = 0, idle = 0;

     // Read the first line of CPU stats from /proc/stat into these variables
    fscanf(fp, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);
    fclose(fp); // Close the file after reading to free resources

    // Static variables to store the previous values between function calls
    static unsigned long prev_total = 0, prev_idle = 0;

    // Calculate total time and idle time difference from previous values
    unsigned long total = user + nice + system + idle;
    unsigned long diff_total = total - prev_total;          // Difference in total time since last call
    unsigned long diff_idle = idle - prev_idle;             // Difference in idle time since last call

    // Update previous values to current values for next call
    prev_total = total;
    prev_idle = idle;

    // Calculate CPU usage percentage (1 - idle time / total time)
    return (1.0 - (float)diff_idle / diff_total) * 100;
}

// Function to display CPU and memory usage on the screen
void display_stats() {
    float cpu_usage = get_cpu_usage();         // Get current CPU usage percentage 
    unsigned long mem_total, mem_available;
    get_memory_usage(&mem_total, &mem_available);   // Get memory total and available values

    // Print CPU usage at line 1, column 0 in green
    attron(COLOR_PAIR(1));                     // Turn on color pair 1 (green)
    mvprintw(1, 0, "CPU Usage: %.2f%%", cpu_usage);
    attroff(COLOR_PAIR(1));                    // Turn off color pair 1

    // Print memory usage at line 2, column 0 in purple
    attron(COLOR_PAIR(2));                     // Turn on color pair 2 (purple)
    mvprintw(2, 0, "Memory Usage: %lu MB / %lu MB",
             (mem_total - mem_available) / 1024, mem_total / 1024);
    attroff(COLOR_PAIR(2));                    // Turn off color pair 2
}

// Function to display a list of running processes from /proc
void display_process_list(int start_row) {
    DIR *dir = opendir("/proc");            // Open /proc directory, where each folder is a process
    if (!dir) return;                       // If directory can't be opened, return early

    struct dirent *entry;                   // Struct for reading each directory entry
    int row = start_row;                    // Start displaying processes from given row

    // Read each entry in /proc directory
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(*entry->d_name)) {          // Only consider entries that are digits (PIDs)
            char path[250];
            snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name); // Build path to process stat file

            FILE *fp = fopen(path, "r");       // Open the stat file for each process
            if (!fp) continue;                 // If file can't be opened, skip this entry

            int pid;
            char comm[250];
            fscanf(fp, "%d %s", &pid, comm);    // Read process ID (PID) and command name (comm)      
            fclose(fp);                         // Close the file

             // Set color for PID (red)
            attron(COLOR_PAIR(3));
            mvprintw(row, 0, "PID: %d,", pid);
            attroff(COLOR_PAIR(3));

            // Display process ID and command name at current row
            mvprintw(row++, 0, "PID: %d, Command: %s", pid, comm);
            if (row > start_row + 10) break; // Limit to 10 processes for simplicity
        }
    }
    
    closedir(dir); // Close the /proc directory after reading
}

// Main function that sets up and runs the program
int main() {
    init_ncurses();     // Initialize ncurses for UI

    while (1) {         // Initiate loop to refresh data every second
        clear();            // Clear the screen to prevent overlapping text
        display_stats();    // Display CPU and memory usage
        display_process_list(4);    // Display processes, start from row 4
        refresh();                  // Refresh the screen to show updates

        int ch = getch();           // Get keyboard input
        if (ch == 'q') break;       // Exit loop if 'q' is pressed
    }
    
    close_ncurses();        // End ncurses mode and restore terminal settings
    return 0;       // Exit
}
