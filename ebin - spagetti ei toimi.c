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
}  

void close_ncurses() {
    endwin();                       // Ends ncurses mode, restores normal terminal behavior
}

// Function to calculate CPU usage from /proc/stat
float get_cpu_usage() {
    // Open the file that holds cpu statistics
    FILE *fp = fopen("proc/stat", "r"); // Open /proc/stat in read mode
    if (!fp)    return -1;              // If the file can't be openend, return -1 as an error

    // Declare variables to store CPU time values
    unsigned long user = 0, nice = 0, system = 0, idle = 0;

     // Read the first line of CPU stats from /proc/stat into these variables
    fscanf(fp, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);
    fclose(fp); // Close the file after reading to free resources

    // Static variables to store the previous values between function calls
    // These are initialized only once, and keep their values between function calls
    static unsigned long prev_total = 0, prev_idle = 0;

    // calculate total time and idle time difference from previous values
    unsigned long total = user + nice + system + idle;
    unsigned long diff_total = total - prev_total;          // Difference in total time since last call
    unsigned long diff_idle = idle - prev_idle;             // Difference in idle time since last call

    // update previous values to current values for next call
    prev_total = total;
    prev_idle = idle;

    // calculate cpu usage percentage (1 - idle time / total time)
    return (1.0 - (float)diff_idle / diff_total) * 100;

}

// Function to display CPU and memory usage on the screen
void display_stats() {
    float cpu_usage = get_cpu_usage();         // Get current CPU usage percentage 
    unsigned long mem_total, mem_available;
    get_memory_usage(&mem_total, &mem_available);   // Get memory total and available values

    // Print CPU usage at line 1, column 0
    mvprintw(1, 0, "CPU Usage: %.2f%%", cpu_usage);

    // Print memory usage at line 2, column 0, converting from KB to MB
    mvprintw(2, 0, "Memory Usage: %lu MB / %lu MB",
             (mem_total - mem_available) / 1024, mem_total / 1024);
}

// function to display a list of running processes from /proc
void display_process_list(int start_row) {
    DIR *dir = opendir("/proc");            // open /proc directory, where each folder is a process
    if (!dir) return;                       // if directory cant be opened, return early

    struct dirent *entry;                   // struct for reading each directory entry
    int row = start_row;                    // start displaying processes from given row

    // read each entry in /proc directory
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(*entry->d_name)) {          // Only consider entries that are digits (PIDs)
            char path[250];
            snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name); // Build path to process stat file

            FILE *fp = fopen(path, "r");       // Open the stat file for each process
            if (!fp) continue;                 // if file cant be opened, skip this entry

            int pid;
            char comm[250];
            fscanf(fp, "%d %s", &pid, comm);    // Read process ID (PID) and command name (comm)      
            fclose(fp);                         // close the file

            // display process ID and command name at current row
            mvprintw(row++, 0, "PID: %d, command: %s", pid, comm);
            if (row > start_row + 10) break; // limit to 10 processes for simplicity
        }
    }
    
    closedir(dir); // close the /proc directory after reading
}

// main function that sets up and runs the program
int main() {
    init_ncurses();     // initialize ncurses for UI

    while (1) {         // initiate loop to refresh data every second
    clear();            // clear the screen to prevent overlapping text
    display_stats();    // display cpu and memory usage
    display_process_list(4);    // display processes, start from row 4
    refresh();                  // refresgh the screen to show updates

    }
    
    close_ncurses();        // end ncurses mode and restore terminal settings
    return 0;       // exit
}