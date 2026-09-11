
/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "../include/process.h"
#include "idt.h"
#include "pic.h"
#include "scheduler.h"
#include "thread.h"
#include "mutex.h"

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_version(void);
static void cmd_colour(const char *args);
static void cmd_halt(void);
static void cmd_ps(void);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void k_utoa(uint32_t value, char *buf) {
    char tmp[11];
    int i = 0;
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (value > 0) {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

static const char *state_name(proc_state_t s) {
    switch (s) {
        case READY:      return "READY";
        case RUNNING:    return "RUNNING";
        case BLOCKED:    return "BLOCKED";
        case TERMINATED: return "TERMINATED";
        default:         return "UNKNOWN";
    }
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  mem     – Memory map (stub)\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      – [L09] List processes\n");
    vga_puts("  kill    – [L09] Terminate a process\n");
    vga_puts("  threads – [L10] List kernel threads\n");
    vga_puts("  free    – [L11] Show free memory\n");
    vga_puts("  ls      – [L12] List files\n");
    vga_puts("  cat     – [L12] Print file contents\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF  :  BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF     :  VGA frame buffer\n");
    vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
                   VGA_YELLOW, VGA_BLACK);
}

static void cmd_ps(void) {
    pcb_t *table = get_process_table();
    int count = get_process_count();

    vga_puts_color("\n  PID  STATE      NAME\n", VGA_LIGHT_CYAN, VGA_BLACK);
        vga_puts("  ---  ---------  --------\n");

    for (int i = 0; i < count; i++) {
        char pidbuf[12];
        k_utoa(table[i].pid, pidbuf);

        vga_puts("  ");
        vga_puts(pidbuf);
        vga_puts("    ");
        vga_puts(state_name(table[i].state));
        vga_puts("   ");
        if (table[i].name[0] != '\0') {
            vga_puts(table[i].name);
        } else {
            vga_puts("(unnamed)");
        }
        vga_puts("\n");
    }
    vga_puts("\n");
}

static void cmd_version(void) {
    vga_puts_color("\n  SENG21213-OS\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  Version : Stage 0\n");
    vga_puts("  Build   : Kernel Foundations\n\n");
}

static void cmd_colour(const char *args) {
    const char *p = k_ltrim(args);
    int fg = 0, bg = 0;
    while (*p >= '0' && *p <= '9') { fg = fg * 10 + (*p - '0'); p++; }
    p = k_ltrim(p);
    while (*p >= '0' && *p <= '9') { bg = bg * 10 + (*p - '0'); p++; }
    vga_set_color((uint8_t)fg, (uint8_t)bg);
    vga_puts("  Colour changed.\n");
}

static void cmd_halt(void) {
    vga_puts_color("\n  System halting...\n", VGA_LIGHT_RED, VGA_BLACK);
    __asm__ __volatile__("cli; hlt");
}
/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }

       if (k_strcmp(cmd, "version") == 0) { cmd_version(); continue; }
       if (k_strncmp(cmd, "colour ", 7) == 0) { cmd_colour(k_ltrim(cmd + 7)); continue; }
       if (k_strcmp(cmd, "halt") == 0) { cmd_halt(); continue; } 
       if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

        /* Milestone stubs */
        if (k_strcmp(cmd, "ps") == 0) { cmd_ps(); continue; }
        if ( k_strcmp(cmd, "kill")    == 0 ||
            k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Stage 1 test tasks (temporary — for isolated context_switch testing)
 * --------------------------------------------------------------------------*/
static void task_a(void) {
    while (1) {
        vga_puts_color("A", VGA_LIGHT_GREEN, VGA_BLACK);
        for (volatile int i = 0; i < 2000000; i++);
    }
}

static void task_b(void) {
    while (1) {
        vga_puts_color("B", VGA_LIGHT_CYAN, VGA_BLACK);
        for (volatile int i = 0; i < 2000000; i++);
    }
}

static void thread_x(void *arg) {
    (void)arg;
    for (int i = 0; i < 10; i++) {
        vga_puts_color("X", VGA_LIGHT_RED, VGA_BLACK);
        for (volatile int j = 0; j < 500000; j++);
        thread_yield();
    }
}

static void thread_y(void *arg) {
    (void)arg;
    for (int i = 0; i < 10; i++) {
        vga_puts_color("Y", VGA_YELLOW, VGA_BLACK);
        for (volatile int j = 0; j < 500000; j++);
        thread_yield();
    }
}

/* ---------------------------------------------------------------------------
 * Race condition demo — shared counter, with and without a mutex
 * --------------------------------------------------------------------------*/
static volatile int race_counter = 0;
static mutex_t race_mutex;
static int race_use_mutex = 0;   /* toggled per run */

static void racer_a(void *arg) {
    (void)arg;
    for (int i = 0; i < 20; i++) {
        if (race_use_mutex) mutex_lock(&race_mutex);
        int temp = race_counter;
        thread_yield();
        race_counter = temp + 1;
        if (race_use_mutex) mutex_unlock(&race_mutex);
        thread_yield();
    }
}
static void racer_b(void *arg) {
    (void)arg;
    for (int i = 0; i < 20; i++) {
        if (race_use_mutex) mutex_lock(&race_mutex);
        int temp = race_counter;
        thread_yield();
        race_counter = temp + 1;
        if (race_use_mutex) mutex_unlock(&race_mutex);
        thread_yield();
    }
}
/* ---------------------------------------------------------------------------
 * Producer-Consumer demo — bounded ring buffer, synchronised with mutex
 * --------------------------------------------------------------------------*/
#define PC_BUF_SIZE 5
#define PC_ITEMS    15
static int pc_buffer[PC_BUF_SIZE];
static int pc_head = 0, pc_tail = 0, pc_count = 0;
static mutex_t pc_mutex;
static int pc_overflow_seen  = 0;
static int pc_underflow_seen = 0;

static void producer_thread(void *arg) {
    (void)arg;
    for (int i = 0; i < PC_ITEMS; i++) {
        int placed = 0;
        while (!placed) {
            mutex_lock(&pc_mutex);
            if (pc_count < PC_BUF_SIZE) {
                pc_buffer[pc_tail] = i;
                pc_tail = (pc_tail + 1) % PC_BUF_SIZE;
                pc_count++;
                placed = 1;
            } else if (pc_count > PC_BUF_SIZE) {
                pc_overflow_seen = 1;
            }
            mutex_unlock(&pc_mutex);
            thread_yield();
        }
    }
}

static void consumer_thread(void *arg) {
    (void)arg;
    for (int i = 0; i < PC_ITEMS; i++) {
        int taken = 0;
        while (!taken) {
            mutex_lock(&pc_mutex);
            if (pc_count > 0) {
                pc_head = (pc_head + 1) % PC_BUF_SIZE;
                pc_count--;
                taken = 1;
            } else if (pc_count < 0) {
                pc_underflow_seen = 1;
            }
            mutex_unlock(&pc_mutex);
            thread_yield();
        }
    }
}

static void pc_demo_process(void) {
    pic_mask_irq(0);

    pc_head = 0; pc_tail = 0; pc_count = 0;
    pc_overflow_seen = 0; pc_underflow_seen = 0;
    mutex_init(&pc_mutex);
    thread_init();
    thread_create(producer_thread, 0);
    thread_create(consumer_thread, 0);
    thread_yield();
    while (thread_current() != NULL) { thread_yield(); }

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_printf("\n  [Producer-Consumer] items=%d  final buffer count=%d  (expected 0)\n", PC_ITEMS, pc_count);
    if (pc_overflow_seen || pc_underflow_seen) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_printf("  [Producer-Consumer] CORRUPTION DETECTED overflow=%d underflow=%d\n\n", pc_overflow_seen, pc_underflow_seen);
    } else {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_printf("  [Producer-Consumer] OK -- no overflow or underflow\n\n");
    }
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    pic_unmask_irq(0);

    while (1) {
        for (volatile int i = 0; i < 2000000; i++);
    }
}

static void thread_demo_process(void) {
    pic_mask_irq(0);   /* block the timer IRQ for the whole demo */

    thread_init();
    thread_create(thread_x, 0);
    thread_create(thread_y, 0);
    thread_yield();

    race_counter = 0;
    race_use_mutex = 0;
    thread_init();
    thread_create(racer_a, 0);
    thread_create(racer_b, 0);
    thread_yield();
    while (thread_current() != NULL) { thread_yield(); }
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_printf("\n  [Race demo] WITHOUT mutex, counter = %d  (expected 40)\n", race_counter);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    for (volatile long d = 0; d < 30000000; d++);

    race_counter = 0;   /* reset before run 2 */
    race_use_mutex = 1;
    mutex_init(&race_mutex);
    thread_init();
    thread_create(racer_a, 0);
    thread_create(racer_b, 0);
    thread_yield();
    while (thread_current() != NULL) { thread_yield(); }
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_printf("  [Race demo] WITH mutex,    counter = %d  (expected 40)\n\n", race_counter);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    for (volatile long d = 0; d < 30000000; d++);
    pic_unmask_irq(0);   /* re-enable the timer now that the demo is done */


    while (1) {
        for (volatile int i = 0; i < 2000000; i++);
    }
}

void kernel_main(void) {
    vga_init();
    kb_init();
    print_splash();
     /* --- Interrupt-driven scheduler setup --- */
    idt_init();
    pic_remap();
    scheduler_init();

    process_init();
    process_create(task_a);
    process_create(task_b);
    process_create(thread_demo_process);
    process_create(pc_demo_process);
    process_create(shell_run);

    __asm__ __volatile__("sti");   /* enable interrupts globally, last */

    
    /* Idle here — the timer now drives task_a/task_b interleaving */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
