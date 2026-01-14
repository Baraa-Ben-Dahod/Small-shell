#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
extern pid_t smash_fg_pid;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    std::cout << "smash: got ctrl-C" << std::endl;

    if (smash_fg_pid != 0) {
        int ret = kill(smash_fg_pid, SIGKILL);

        if (ret == -1) {
            perror("smash error: kill failed");
            return;
        }

        std::cout << "smash: process " << smash_fg_pid << " was killed" << std::endl;

        smash_fg_pid = 0;
    }
}
