# Smash - Small Shell

![C++](https://img.shields.io/badge/Language-C++11-blue.svg) ![Platform](https://img.shields.io/badge/Platform-Linux-orange.svg) ![Course](https://img.shields.io/badge/Course-Operating_Systems-green.svg)

**A robust, Unix-like shell implementation written in C++ designed to manage processes, handle system signals, and execute commands using low-level system calls.**

## 📖 Overview

**Smash** (Small Shell) is a custom command-line interpreter that mimics the behavior of a real Linux shell (like Bash). This project demonstrates a deep understanding of Operating Systems concepts, including process management, inter-process communication (IPC), signal handling, and file system manipulation.

Built as part of the **Operating Systems (02340123)** course at the **Technion - Israel Institute of Technology**.

## 🚀 Key Features

### 1. Process Management (Job Control)
* **Foreground & Background Execution:** Supports running commands in the background using `&` and bringing them to the foreground.
* **Job Monitoring:** The `jobs` command lists all currently running background processes with their specific Job IDs.
* **Process Termination:** Ability to kill specific jobs or all running processes upon quitting.

### 2. I/O Redirection & Piping
* **Redirection:** Supports overwriting (`>`) and appending (`>>`) output to files.
* **Piping:** Implements standard piping (`|`) to pass stdout to stdin, and error piping (`|&`) to pass stderr.

### 3. Signal Handling
* **Ctrl-C (SIGINT):** Custom handler that terminates the currently running foreground process without killing the shell itself.

### 4. Advanced System & File Commands
* **`usbinfo`:** Scans the internal file system (`/sys/bus/usb/devices`) to list connected USB devices and their power consumption (Bonus).
* **`sysinfo`:** Retrieves kernel version, hostname, and uptime using system calls.
* **`du`:** Recursively calculates disk usage for a directory.
* **`whoami`:** Displays current user information (UID, GID, Home Dir).

### 5. Shell Built-in Utilities
* `chprompt`: Change the shell prompt text.
* `showpid`: Display the shell's process ID.
* `pwd` / `cd`: Navigate the file system (handling `cd -` for previous directory).
* `alias` / `unalias`: Create and remove shortcuts for commands.
* `unsetenv`: Remove environment variables directly from memory.

## 🛠 Technical Highlights

This project serves as a showcase for:
* **System Calls:** Extensive use of `fork()`, `execvp()`, `waitpid()`, `pipe()`, `dup2()`, `kill()`, `getdents64`, and `signal()`.
* **Design Patterns:**
    * **Singleton:** Used for the `SmallShell` class to ensure a single instance manages the session.
    * **Factory Pattern:** Implemented in `CreateCommand` to generate specific command objects polymorphically.
* **C++ & OOP:** Strong usage of inheritance (`Command` base class), polymorphism, and STL containers.
* **Memory Management:** Proper handling of dynamic memory and resources.

## 💻 Installation & Usage

### Prerequisites
* Linux Environment (Ubuntu/Debian recommended).
* G++ Compiler (Supporting C++11).
* Make.

### Build
To compile the project, navigate to the root directory and run:

```bash
make

```

### Run

Start the shell:

```bash
./smash

```

### Usage Examples

**1. Basic Commands & Aliases**

```bash
smash> chprompt my_shell
my_shell> alias l='ls -l'
my_shell> l
# Lists files in long format...

```

**2. Background Jobs & Piping**

```bash
smash> sleep 100 &
[1] sleep 100&
smash> jobs
[1] sleep 100&
smash> ls | grep Makefile
Makefile

```

**3. System Info**

```bash
smash> usbinfo
Device 1: ID 0781:5583 SanDisk Ultra Fit MaxPower: 200mA

```

## 📂 Project Structure

* `smash.cpp`: Main entry point containing the event loop.
* `Commands.h/cpp`: Implementation of the Command classes, Factory, and built-in logic.
* `signals.h/cpp`: Signal handling logic (Ctrl+C).
* `Makefile`: Compilation rules.

## 👥 Authors

* **Baraa Ben Dahod**
