#include <unistd.h>
#include <string.h>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fstream>
#include <sys/utsname.h>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cmath>
#include <pwd.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <algorithm>
#include <sys/syscall.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const char ExternalCommand::WILDCARDS[] = {'*', '?'};

// Helper functions and declarations
extern pid_t smash_fg_pid;
extern char **environ;
struct UsbDevice;
struct linux_dirent64;
static bool isStringRepValidNum(const char* str);
static long long calculateDirectorySize(const std::string& path);
static void addUsbDevice(const std::string& path, std::vector<UsbDevice>& devices);
static std::string read_content(const std::string& path);

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;
    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h
// TODO: SmallShell class

SmallShell::SmallShell() : m_prompt("smash> "), m_lastPwd(NULL) {}

SmallShell::~SmallShell() {
    if (m_lastPwd != NULL) {
        free(m_lastPwd);
    }
}

std::string SmallShell::resolveAlias(const char *cmd_line) const {
    std::string original_line = _trim(std::string(cmd_line));
    if (original_line.empty()) {
        return "";
    }

    const size_t space_pos = original_line.find_first_of(" \t\n&|>");
    // the command line consists of one word
    const std::string first_word = (space_pos == std::string::npos)
                             ? original_line
                             : original_line.substr(0, space_pos);

    std::string substituted_command;
    for(const auto& alias_pair : m_aliases) {
        if (alias_pair.first == first_word) {
            substituted_command = alias_pair.second;
            break;
        }
    }

    // No alias found, return the original command line
    if (substituted_command.empty()) {
        return original_line;
    }

    // If alias is found construct the new command line
    std::string new_cmd_line = substituted_command;
    if (space_pos != std::string::npos) {
        const std::string rest_of_line = _trim(original_line.substr(space_pos));
        if (!rest_of_line.empty()) {
            new_cmd_line += " " + rest_of_line;
        }
    }

    return new_cmd_line;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    std::string cmd_s = _trim(cmd_line);
    if (cmd_s.empty()) {
        return nullptr;
    }

    // Special command: Check for Pipe character
    if (cmd_s.find('|') != std::string::npos) {
        return new PipeCommand(cmd_line);
    }

    // Special command: Check for IO Redirection character
    else if (cmd_s.find('>') != std::string::npos) {
        return new RedirectionCommand(cmd_line);
    }

    std::string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));

    // Built-in commands' factory
    if (firstWord == "chprompt") {
        return new ChangePromptCommand(cmd_line, this);
    }
    else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line, &m_lastPwd);
    }
    else if (firstWord == "alias") {
        return new AliasCommand(cmd_line);
    }
    else if (firstWord == "unalias") {
        return new UnAliasCommand(cmd_line);
    }
    else if (firstWord == "unsetenv") {
        return new UnSetEnvCommand(cmd_line);
    }
    else if (firstWord == "sysinfo") {
        return new SysInfoCommand(cmd_line);
    }
    else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "fg") {
        return new ForegroundCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "quit") {
        return new QuitCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "kill") {
        return new KillCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "du") {
        return new DiskUsageCommand(cmd_line);
    }
    else if (firstWord == "whoami") {
        return new WhoAmICommand(cmd_line);
    }
    else if (firstWord == "usbinfo") {
        return new USBInfoCommand(cmd_line);
    }
    // External commands' factory
    else {
        return new ExternalCommand(cmd_line);
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    if (strlen(cmd_line) == 0) return;

    // Must remove any finished jobs before executing any command
    m_jobsList.removeFinishedJobs();
    // Determine the command
    const std::string cmd_line_resolved = resolveAlias(cmd_line);
    const char* real_cmd_line = cmd_line_resolved.c_str();

    // removing the & sign
    bool is_background_intent = _isBackgroundComamnd(real_cmd_line);
    char removed_background_cmd_line[COMMAND_MAX_LENGTH + 1];
    strcpy(removed_background_cmd_line, real_cmd_line);
    if (is_background_intent) {
        _removeBackgroundSign(removed_background_cmd_line);
    }


    Command* cmd_obj = CreateCommand(removed_background_cmd_line);
    if (cmd_obj == nullptr) {
        return;
    }

    // Execute for Built-in Commands or Special Commands
    if (!dynamic_cast<ExternalCommand*>(cmd_obj)) {
        // Ignore &
        cmd_obj->execute();
        delete cmd_obj;
    }
    else {
        delete cmd_obj;
        cmd_obj = new ExternalCommand(real_cmd_line);
        // This will handle background commands too
        cmd_obj->execute();
        delete cmd_obj;
    }
}

void SmallShell::showPrompt() const {
    std::cout << m_prompt;
}

void SmallShell::setPrompt(const std::string& newPrompt) {
    m_prompt = newPrompt;
}

std::vector<std::pair<std::string, std::string>>& SmallShell::getAliases() {
    return getInstance().m_aliases;
}

JobsList& SmallShell::getJobsList() {
    return m_jobsList;
}


// Command class
Command::Command(const char* cmd_line, bool parse_args) : m_cmd_line(cmd_line), m_cmd_args{},
          m_num_args(parse_args ? _parseCommandLine(cmd_line, m_cmd_args) : 0) {}

Command::~Command() {
    for (int i = 0; i < m_num_args; ++i) {
        free(m_cmd_args[i]);
    }
}

const string& Command::getCmdLine() const {
    return m_cmd_line;
}

// BuiltInCommand class
BuiltInCommand::BuiltInCommand(const char* cmd_line, bool parse_args) : Command(cmd_line, parse_args) {}

// ExternalCommand class

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line, false) {
    strcpy(m_removed_background_cmd_line, m_cmd_line.c_str());
    _removeBackgroundSign(m_removed_background_cmd_line);
    m_num_args = _parseCommandLine(m_removed_background_cmd_line, m_cmd_args);
}

void ExternalCommand::execute() {
    bool isComplex = is_complex_command();
    pid_t pid = fork();
    if(pid < 0) {
        perror("smash error: fork failed");
        return;
    }

    // For the child
    if(pid == 0) {
        setpgrp();
        char *argv[] = {
                const_cast<char*>("/bin/bash"),
                const_cast<char*>("-c"),
                const_cast<char*>(m_removed_background_cmd_line),
                NULL
        };

        const char* path =  isComplex ? "/bin/bash" : m_cmd_args[0];
        char** ArgList = isComplex ? argv : m_cmd_args;
        execvp(path, ArgList);
        perror("smash error: execvp failed");
        exit(1);
    }

    // For the parent-smash process
    const char* cmd_line = m_cmd_line.c_str();
    SmallShell& smash = SmallShell::getInstance();


    if(_isBackgroundComamnd(cmd_line)) {
        smash.getJobsList().addJob(this, pid, false);
    } else {
        smash_fg_pid = pid;
        int status;
        if (waitpid(pid, &status, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            smash_fg_pid = 0;
            return;
        }

        smash_fg_pid = 0;

        if (WIFSTOPPED(status)) {
            smash.getJobsList().addJob(this, pid, true);
        }
    }
}

bool ExternalCommand::is_complex_command() const {
    return (m_cmd_line.find(WILDCARDS[0]) != std::string::npos ||
            m_cmd_line.find(WILDCARDS[1]) != std::string::npos);
}


// chprompt command
ChangePromptCommand::ChangePromptCommand(const char *cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, false), m_sshell(smash) {
    // find first parameter and ignore the rest of the command line
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    string remaining = cmd_s.substr(firstWord.length());
    string newPrompt = _trim(remaining);
    newPrompt = newPrompt.substr(0, newPrompt.find_first_of(" \n"));
    if(newPrompt.empty()) {
        newPrompt = "smash> ";
    } else {
        newPrompt = newPrompt + "> ";
    }
    this->m_prompt = newPrompt;
}

void ChangePromptCommand::execute() {
    m_sshell->setPrompt(m_prompt);
}

// showpid command
ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line, false) {}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}

// pwd command
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line, false) {}

void GetCurrDirCommand::execute() {
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX) != NULL) {
        std::cout << buffer << std::endl;
    } else {
        perror("smash error: getcwd failed");
    }
}

// cd command
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line, true), m_plastPwd(plastPwd)
{}

void ChangeDirCommand::execute() {
    if(m_num_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        return;
    }

    m_targetPath = (m_num_args == 2) ? std::string(m_cmd_args[1]) : "";

    if(m_targetPath.empty()) {
        return;
    }

    char curr_path[PATH_MAX];

    if (getcwd(curr_path, PATH_MAX) == NULL) {
        perror("smash error: getcwd failed");
        return;
    }

    std::string target_path = m_targetPath;

    if(m_targetPath == "-") {
        if(*m_plastPwd == NULL || strlen(*m_plastPwd) == 0) {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            return;
        }
        target_path = std::string(*m_plastPwd);
    }

    if(chdir(target_path.c_str()) == -1) {
        perror("smash error: chdir failed");
        return;
    }

    if (*m_plastPwd != NULL) {
        free(*m_plastPwd);
    }

    *m_plastPwd = strdup(curr_path);
}

// jobs command

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line, false),
m_jobsList(jobs) {}


void JobsCommand::execute() {
    // Finished jobs will be removed inside printJobsList()
    m_jobsList->printJobsList();
}

// fg command
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line, true),
m_jobsList(jobs){}


void ForegroundCommand::execute() {
    if(m_num_args > 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    int job_id = m_jobsList->getMaxJobId();

    // Check if the arguments represent a valid number
    if (m_num_args == 2) {
        if(!isStringRepValidNum(m_cmd_args[1])) {
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return;
        }
        job_id = std::stoi(m_cmd_args[1]);
    }

    JobsList::JobEntry* job = m_jobsList->getJobById(job_id);

    if(job == nullptr && m_num_args == 2) {
        std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
        return;
    }

    // No argument specified, hence print empty list error
    if(m_jobsList->empty()) {
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
        return;
    }

    pid_t job_pid = job->getJobPID();
    std::string cmd_line = job->getCmdLine();

    std::cout << job->getCmdLine() << " " << job_pid <<  std::endl;

    m_jobsList->removeJobById(job_id);
    smash_fg_pid = job_pid;

    if (kill(job_pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
        smash_fg_pid = 0;
        return;
    }

    int status;
    if (waitpid(job_pid, &status, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        smash_fg_pid = 0;
        return;
    }

    if (WIFSTOPPED(status)) {
        m_jobsList->addJob(std::move(cmd_line), job_pid, true);
    }
    smash_fg_pid = 0;

}

// quit command

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line, true), m_jobs_list(jobs) {
    if(m_num_args > 1 && strcmp(m_cmd_args[1], "kill") == 0) {
        m_is_kill_specified = true;
    }
}

void QuitCommand::execute() {
    if (m_is_kill_specified) {
        m_jobs_list->killAllJobs();
    }
    // The QuitCommand pointer and all process memory will be reclaimed by the OS when calling exit()
    exit(0);
}


// kill command
KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line, true), m_jobsList(jobs){}

void KillCommand::execute() {
    if (m_num_args != 3) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    const char* signum_str = m_cmd_args[1];

    if (signum_str[0] == '-') {
        signum_str = signum_str + 1;
    } else {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    if (!isStringRepValidNum(signum_str) || !isStringRepValidNum(m_cmd_args[2])) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    int signum = std::stoi(signum_str);
    int job_id = std::stoi(m_cmd_args[2]);
    auto job = m_jobsList->getJobById(job_id);

    if (job == nullptr) {
        std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
        return;
    }

    pid_t job_pid = job->getJobPID();

    if (kill(job_pid, signum) == -1) {
        perror("smash error: kill failed");
        return;
    }
    std::cout << "signal number " << signum << " was sent to pid " << job_pid << std::endl;

}

// alias command
const std::unordered_set<std::string> AliasCommand::RESERVED_KEYWORDS = {
        "chprompt", "showpid", "pwd", "cd", "jobs", "fg",
        "quit", "kill", "alias", "unalias", "unsetenv", "sysinfo",
        "du", "whoami", "usbinfo"
};


AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line, true) {}

void AliasCommand::execute() {
    auto& allAlias = SmallShell::getAliases();
    if(m_num_args == 1) {
        for(const auto& al : allAlias) {
            std::cout << al.first << "='" << al.second << "'" << std::endl;
        }
        return;
    }
    extractNameAndCommand();
    if (m_name.empty() || m_command.empty()) {
        // Parsing error already printed inside extractNameAndCommand
        return;
    }
    insertNewAlias(allAlias);
}

void AliasCommand::extractNameAndCommand() {
    std::string cmd_s = _trim(m_cmd_line);
    size_t alias_end_pos = 5;
    size_t definition_start = cmd_s.find_first_not_of(WHITESPACE, alias_end_pos);
    std::string raw_definition = cmd_s.substr(definition_start);
    size_t eq_pos = raw_definition.find('=');
    if (eq_pos == std::string::npos || eq_pos == 0) {
        // '=' must exist and cannot be the first character.
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return;
    }
    // Extract the name
    m_name = _trim(raw_definition.substr(0, eq_pos));
    // Extract the command part
    std::string command_part = raw_definition.substr(eq_pos + 1);
    if (command_part.length() < 2 || command_part.front() != '\'' || command_part.back() != '\'') {
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return;
    }
    // Extract the command
    m_command = command_part.substr(1, command_part.length() - 2);
}

bool AliasCommand::isValidName() const {
    // Assuming the name is not empty
    if (RESERVED_KEYWORDS.count(m_name) > 0) {
        std::cerr << "smash error: alias: " << m_name << " already exists or is a reserved command" << std::endl;
        return false;
    }

    for(const auto& name_pair: SmallShell::getAliases()) {
        if(name_pair.first == m_name) {
            std::cerr << "smash error: alias: " << m_name << " already exists or is a reserved command" << std::endl;
            return false;
        }
    }
    return true;
}

bool AliasCommand::isLegalName() const {
    if(m_name.empty()) {
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return false;
    }
    for(char c: m_name) {
        if (!std::isalnum(c) && c != '_') {
            std::cerr << "smash error: alias: invalid alias format" << std::endl;
            return false;
        }
    }
    return true;
}

void AliasCommand::insertNewAlias(std::vector<std::pair<std::string, std::string>>& allAlias) const {
    if(isValidName() && isLegalName()) {
            allAlias.emplace_back(m_name, m_command);
    }
}

// unalias command
UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line, true) {}

void UnAliasCommand::execute() {
    if(m_num_args <= 1) {
        std::cerr << "smash error: unalias: not enough arguments" << std::endl;
        return;
    }

    auto& allAlias = SmallShell::getAliases();
    for(int i = 1; i < m_num_args; i++) {
        std::string alias_to_delete = m_cmd_args[i];
        bool isFound = false;

        for (auto it = allAlias.begin(); it != allAlias.end(); ++it) {
            if (it->first == alias_to_delete) {
                allAlias.erase(it);
                isFound = true;
                break;
            }
        }
        if(!isFound) {
            std::cerr << "smash error: unalias: " << alias_to_delete << " alias does not exist" << std::endl;
            return;
        }
    }

}

// unsetenv command
UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line, true) {}

void UnSetEnvCommand::execute() {
    if(m_num_args <= 1) {
        std::cerr << "smash error: unsetenv: not enough arguments" << std::endl;
        return;
    }
    pid_t smash_pid = getpid();
    std::string env_path = "/proc/" + std::to_string(smash_pid) + "/environ";
    int env_file = open(env_path.c_str(), O_RDONLY);

    if (env_file == -1) {
        perror("smash error: open failed");
        return;
    }

    for(int i = 1; i < m_num_args; i++) {
        lseek(env_file, 0, SEEK_SET);
        std::string var_name = m_cmd_args[i];
        bool found_in_file = false;

        char buffer[4096];
        ssize_t bytes = read(env_file, buffer, sizeof(buffer));

        if(bytes == -1) {
            perror("smash error: read failed");
            close(env_file);
            return;
        }

        int curr_pos = 0;
        while(curr_pos < bytes) {
            // This will read until encounter null terminator
            std::string entry(buffer + curr_pos);

            if (entry.find(var_name + "=") == 0) {
                found_in_file = true;
                break;
            }

            curr_pos += entry.size() + 1;
        }

        if(!found_in_file) {
            std::cerr << "smash error: unsetenv: " << var_name << " does not exist" << std::endl;
            close(env_file);
            return;
        }

        // Removing from char **__environ array:
        for(int j = 0; environ[j] != NULL; j++) {
            std::string key_prefix = var_name + "=";
            if (strncmp(environ[j], key_prefix.c_str(), key_prefix.length()) == 0) {
                // Shifting the array
                for(int k = j; environ[k] != NULL; k++) {
                    environ[k] = environ[k + 1];
                }
                break; // variable is found and removed
            }
        }
    }

    close(env_file);
}

// sysinfo command

SysInfoCommand::SysInfoCommand(const char *cmd_line) : BuiltInCommand(cmd_line, false) {}

void SysInfoCommand::execute() {
    struct utsname uts;
    if(uname(&uts) == -1) {
        perror("smash error: uname failed");
        return;
    }

    struct sysinfo info;
    if(sysinfo(&info) == -1) {
        perror("smash error: sysinfo failed");
        return ;
    }

    time_t boot_time = time(NULL) - info.uptime;
    struct tm *tm_info = localtime(&boot_time);

    /*
     * "%Y-%m-%d %H:%M:%S" Format needs at most:
     * 4 bytes for Year (4 digits e.g, 2025)
     * 2 bytes for Month (2 digits : 0-12)
     * 2 bytes for Days (2 digits : 0-31)
     * 2 bytes for Hours (2 digits : 0-23)
     * 2 bytes for Minutes (2 digits : 0-59)
     * 2 bytes for Seconds (2 digits : 0-59)
     * 1 byte for each char: '-' twice, ' ' once, ':' twice
     * 1 byte for null terminator
     * Overall it needs 20 bytes - Use 24 for more safety.
    */
    char buffer_time[24];
    strftime(buffer_time, sizeof(buffer_time), "%Y-%m-%d %H:%M:%S", tm_info);

    std::cout << "System: " << uts.sysname << std::endl;
    std::cout << "Hostname: " << uts.nodename << std::endl;
    std::cout << "Kernel: " << uts.release << std::endl;
    std::cout << "Architecture: " << uts.machine << std::endl;
    std::cout << "Boot Time: " << buffer_time << std::endl;
}


// Special commands

// IO redirection command
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line, false)
{
    size_t temp1 = m_cmd_line.find(">>");
    size_t temp2 = m_cmd_line.find('>');
    size_t redirection_pos = (temp1 != std::string::npos) ? temp1 : temp2;
    m_redirection_char = (temp1 != std::string::npos) ? ">>" : ">";

    m_command_line = m_cmd_line.substr(0, redirection_pos);
    m_output_file = m_cmd_line.substr(redirection_pos + m_redirection_char.size());
    m_output_file = _trim(m_output_file);

    strcpy(m_removed_background_cmd_line, m_command_line.c_str());
    _removeBackgroundSign(m_removed_background_cmd_line);

}

void RedirectionCommand::execute() {
    int stdout_backup = dup(1);
    if (stdout_backup == -1) {
        perror("smash error: dup failed");
        return;
    }


    if (close(1) == -1) {
        perror("smash error: close failed");
        dup2(stdout_backup, 1);
        close(stdout_backup);
        return;
    }

    int flags = O_WRONLY | O_CREAT;
    if (m_redirection_char == ">") {
        flags |= O_TRUNC;
    } else {
        flags |= O_APPEND;
    }


    int file = open(m_output_file.c_str(), flags, 0666);
    if(file == -1) {
        perror("smash error: open failed");
        dup2(stdout_backup, 1);
        close(stdout_backup);
        return;
    }


    SmallShell& smash = SmallShell::getInstance();
    smash.executeCommand(m_removed_background_cmd_line);

    close(1);
    if (dup2(stdout_backup, 1) == -1) {
        perror("smash error: dup2 failed");
    }
    close(stdout_backup);

}

// Pipes command
PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line, false) {
    size_t temp1 = m_cmd_line.find("|&");
    size_t temp2 = m_cmd_line.find('|');
    size_t pipe_pos = temp1 != (std::string::npos) ? temp1 : temp2;
    m_pipe_char = temp1 != (std::string::npos) ? "|&" : "|";

    m_command_line1 = m_cmd_line.substr(0, pipe_pos);
    m_command_line2 = m_cmd_line.substr(pipe_pos + m_pipe_char.size());


    strcpy(m_removed_background_command1, m_command_line1.c_str());
    strcpy(m_removed_background_command2, m_command_line2.c_str());
    _removeBackgroundSign(m_removed_background_command1);
    _removeBackgroundSign(m_removed_background_command2);

}

void PipeCommand::execute() {
    int redirected_write_channel = (m_pipe_char == "|") ? 1 : 2;

    int my_pipe[2];
    if(pipe(my_pipe) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    // Implementation for "|" pipe char
    pid_t first_child = fork();
    if (first_child == -1) {
        perror("smash error: fork failed");
        close(my_pipe[0]);
        close(my_pipe[1]);
        return;
    }

    // === First child (writer) ===
    if(first_child == 0) {
        setpgrp();
        close(my_pipe[0]);

        // Redirect stdout to pipe write channel
        if (dup2(my_pipe[1], redirected_write_channel) == -1) {
            perror("smash error: dup2 failed");
            exit(1);
        }

        close(my_pipe[1]);
        SmallShell& smash1 = SmallShell::getInstance();
        smash1.executeCommand(m_removed_background_command1);
        exit(0);
    }


    // Continue parent process
    pid_t second_child = fork();
    if (second_child == -1) {
        perror("smash error: fork failed");
        close(my_pipe[0]);
        close(my_pipe[1]);
        return;
    }


    // === Second child (reader) ===
    if(second_child == 0) {
        setpgrp();
        close(my_pipe[1]);

        // Redirect stdin to pipe read channel
        if (dup2(my_pipe[0], 0) == -1) {
            perror("smash error: dup2 failed");
            exit(1);
        }

        close(my_pipe[0]);
        SmallShell& smash1 = SmallShell::getInstance();
        smash1.executeCommand(m_removed_background_command2);
        exit(0);
    }


    // Parent process
    close(my_pipe[0]);
    close(my_pipe[1]);
    waitpid(first_child, NULL, 0);
    waitpid(second_child, NULL, 0);
}

// du command
DiskUsageCommand::DiskUsageCommand(const char *cmd_line) : Command(cmd_line, true) {}

void DiskUsageCommand::execute() {
    if (m_num_args > 2) {
        std::cerr << "smash error: du: too many arguments" << std::endl;
        return;
    }

    std::string directory_path;

    if (m_num_args == 1) {
        char buffer[COMMAND_MAX_LENGTH];
        if (getcwd(buffer, PATH_MAX) == NULL) {
            perror("smash error: getcwd failed");
            return;
        }
        directory_path = std::string(buffer);
    } else {
        directory_path = std::string(m_cmd_args[1]);
    }

    auto size_in_bytes = calculateDirectorySize(directory_path);
    auto size_in_kb = (size_in_bytes + 1023) / 1024;

    std::cout << "Total disk usage: " << size_in_kb  << " KB" << std::endl;
}

// whoami command
WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line, false) {}

void WhoAmICommand::execute() {
    uid_t user_id = geteuid();

    int file = open("/etc/passwd", O_RDONLY);
    if(file == -1) {
        perror("smash error: open failed");
        return;
    }

    char buffer[4096+1]; // +1 for '\0'
    ssize_t bytes_read = read(file, buffer, sizeof(buffer) - 1);
    close(file);

    if(bytes_read == -1) {
        perror("smash error: read failed");
        return;
    } else if (bytes_read == 0) {
        return;
    }

    buffer[bytes_read] = '\0';

    // Parsing logic
    std::string content(buffer);
    std::stringstream ss(content);
    std::string line;

    while(std::getline(ss, line)) {
        std::stringstream line_ss(line);
        std::string segment;
        std::vector<std::string> fields;

        while (std::getline(line_ss, segment, ':')) {
            fields.push_back(segment);
        }

        if(fields.size() >= 6 && static_cast<uid_t>(std::stoi(fields[2])) == user_id) {
            std::cout << fields[0] << std::endl; // user name
            std::cout << user_id << std::endl; // user id
            std::cout << fields[3] << std::endl; // user id
            std::cout << fields[5] << std::endl; // home dir
            return;
        }
    }
}


// usbinfo command

struct UsbDevice {
    int devNum;
    std::string idVendor;
    std::string idProduct;
    std::string manufacturer;
    std::string product;
    std::string maxPower;

    UsbDevice(int d, std::string ven, std::string id_prod, std::string manf,
              std::string prod, std::string max_pow) :
              devNum(d), idVendor(std::move(ven)),
              idProduct(std::move(id_prod)), manufacturer(std::move(manf)),
              product(std::move(prod)), maxPower(std::move(max_pow)) {}
};

struct linux_dirent64 {
    unsigned long  d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

USBInfoCommand::USBInfoCommand(const char *cmd_line) : Command(cmd_line, false) {}


/*
 * Implementation logic:
 * Work in "/sys/bus/usb/devices/" directory
 * For all folders inside this directory check if it contains the file "devnum"
 * If yes : this is an usb folder ,read all other files:
 * devnum for devnum
 * idVendor for vendor
 * idProduct for product
 * manufacturer for manufacturer
 * product for product name
 * bMaxPower for max power
 * If no: continue
*/

void USBInfoCommand::execute() {
    std::vector<UsbDevice> devices;
    int dir = open("/sys/bus/usb/devices", O_RDONLY | O_DIRECTORY);
    if(dir == -1) {
        perror("smash error: open failed");
        return;
    }

    char buffer[4096];
    int nread;

    while ((nread = syscall(SYS_getdents64, dir, buffer, sizeof(buffer))) > 0) {
        int byte_pos = 0;

        while (byte_pos < nread) {
            auto *dirEntry = (struct linux_dirent64 *) (buffer + byte_pos);

            // The dir entry represents a folder , try reading devnum file inside this entry
            std::string name = dirEntry->d_name;
            if (name != "." && name != "..") {
                const std::string path = "/sys/bus/usb/devices/" + name;
                addUsbDevice(path, devices);
            }

            // Jump to next entry
            byte_pos += dirEntry->d_reclen;
        }
    }

    close(dir);

    if (devices.empty()) {
        std::cerr << "smash error: usbinfo: no USB devices found" << std::endl;
        return;
    }

    // Sort the devices by device number in ascending order
    std::sort(devices.begin(), devices.end(),
              [](const UsbDevice &a, const UsbDevice &b) {
        return a.devNum < b.devNum;
    });


    for (const auto &dev: devices) {
        std::cout << "Device " << dev.devNum << ": ID " << dev.idVendor << ":"
        << dev.idProduct << " " << dev.manufacturer << " " << dev.product << " MaxPower: " <<
        dev.maxPower << "mA" << std::endl;
    }

}




// TODO: JobsList Entry class

JobsList::JobEntry::JobEntry(int job_id, pid_t pid, State curr_state, const Command *cmd) :
   m_jobId(job_id), m_pid(pid), m_cmdLine(cmd->getCmdLine()), m_currentState(curr_state) {}

JobsList::JobEntry::JobEntry(int job_id, pid_t pid, State curr_state, std::string  cmd_line) :
        m_jobId(job_id), m_pid(pid), m_cmdLine(std::move(cmd_line)), m_currentState(curr_state) {}

int JobsList::JobEntry::getJobID() const {
    return m_jobId;
}

pid_t JobsList::JobEntry::getJobPID() const {
    return m_pid;
}

const std::string& JobsList::JobEntry::getCmdLine() const {
    return m_cmdLine;
}



// TODO: JobsList class
/*
void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    int new_job_id = 1;
    if(!m_jobs.empty()) {
        new_job_id = m_jobs.back().getJobID() + 1;
    }
    auto state = isStopped ? State::stopped : State::running;
    m_jobs.emplace_back(new_job_id, pid, state, cmd);
}
 */
void JobsList::addJob(const Command *cmd, pid_t pid, bool isStopped) {
    addJob(cmd->getCmdLine(), pid, isStopped);
}

// May make std::string cmd_line of type const std::string&
void JobsList::addJob(const std::string& cmd_line, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    int new_job_id = 1;
    if(!m_jobs.empty()) {
        new_job_id = m_jobs.back().getJobID() + 1;
    }
    auto state = isStopped ? State::stopped : State::running;
    m_jobs.emplace_back(new_job_id, pid, state, cmd_line);
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    // The vector is already sorted
    for (const auto& job : m_jobs) {
        // Print format: [<job-id>] <command>
        std::cout << "[" << job.getJobID() << "] "
                  << job.getCmdLine() << std::endl;
    }
}

void JobsList::killAllJobs() {
    removeFinishedJobs();
    std::cout << "smash: sending SIGKILL signal to "<< m_jobs.size() << " jobs:" << std::endl;

    for(const auto& job: m_jobs) {
        std::cout << job.getJobPID() << ": " << job.getCmdLine() << std::endl;
        if (kill(job.getJobPID(), SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
    }

    m_jobs.clear();
}

void JobsList::removeFinishedJobs() {
    for (auto it = m_jobs.begin(); it != m_jobs.end(); ) {
        int status;
        pid_t result = waitpid(it->getJobPID(), &status, WNOHANG);

        if (result > 0 || result == -1) {
            it = m_jobs.erase(it);
        } else {
            ++it;
        }

    }
}


JobsList::JobEntry* JobsList::getJobById(int jobId) const {
    for(const auto& job: m_jobs) {
        if(job.getJobID() == jobId) {
            return const_cast<JobEntry *>(&job);
        }
    }
    return nullptr;
}

bool JobsList::empty() const {
    return m_jobs.empty();
}

int JobsList::getMaxJobId() const {
    if(m_jobs.empty()) {
        return -1; // failure
    }
    return m_jobs.back().getJobID();
}

void JobsList::removeJobById(int jobId) {
    for(auto it = m_jobs.begin(); it != m_jobs.end(); ++it) {
        if(it->getJobID() == jobId) {
            m_jobs.erase(it);
            return;
        }
    }
}


// Helper functions

// Returns true if the string represents a valid number
static bool isStringRepValidNum(const char* str) {
    char* end;
    long parsed = std::strtol(str, &end, 10);
    if (*end != '\0' || parsed <= 0) {
        return false;
    }
    return true;
}

// Calculate the directory size
static long long calculateDirectorySize(const std::string& path) {
    struct stat sb;

    if (lstat(path.c_str(), &sb) == -1) {
        perror("smash error: lstat failed");
        return 0;
    }

    // Get the file's size
    auto size_in_bytes = sb.st_size;

    // Determine if it's a regular file or a directory
    if (S_ISDIR(sb.st_mode)) {
        int dir = open(path.c_str(), O_RDONLY | O_DIRECTORY);
        if (dir == -1) {
            perror("smash error: open failed");
            return 0;
        }

        char buffer[4096];
        int nread;

        while ((nread = syscall(SYS_getdents64, dir, buffer, sizeof(buffer))) > 0) {
            int byte_pos = 0;

            while (byte_pos < nread) {
                auto *dirEntry = (struct linux_dirent64 *) (buffer + byte_pos);

                std::string name = dirEntry->d_name;
                if (name != "." && name != "..") {
                    size_in_bytes += calculateDirectorySize(path + "/" + name);
                }

                // Jump to next entry
                byte_pos += dirEntry->d_reclen;
            }
        }

        if (nread == -1) {
            perror("smash error: getdents64 failed");
        }

        close(dir);
        return size_in_bytes;
        
    } else if (S_ISREG(sb.st_mode)) {
        return size_in_bytes;
    }

    // Else it's a symbolic link
    return 0;
}

static std::string read_content(const std::string& path) {
    int file = open(path.c_str(), O_RDONLY);
    if(file == -1) {
        return "N/A";
    }

    char buffer[256+1]; // +1 for '\0'
    ssize_t byte_read = read(file, buffer, sizeof(buffer) - 1);
    close(file);

    if(byte_read <= 0) {
        return "N/A";
    }

    buffer[byte_read] = '\0';
    std::string res(buffer);

    if (!res.empty() && res.back() == '\n') {
        res.pop_back();
    }

    // Removing "mA" from the end (if exists) in /bMaxPower file
    if (res.size() >= 2 && res.substr(res.size() - 2) == "mA") {
        res = res.substr(0, res.size() - 2);
    }

    return res.empty() ? "N/A" : res;
}

void addUsbDevice(const std::string& path, std::vector<UsbDevice>& devices) {
    const std::string devnum_path = path + "/devnum";
    const int file = open(devnum_path.c_str(), O_RDONLY);

    if (file == -1) {
        return;
    }

    close(file);

    const std::string devnum_content = read_content(devnum_path);
    if (!isStringRepValidNum(devnum_content.c_str())) {
        return;
    }

    const int devnum = std::stoi(devnum_content);
    const std::string idVendor = read_content(path + "/idVendor");
    const std::string idProduct = read_content(path + "/idProduct");
    const std::string manufacturer = read_content(path + "/manufacturer");
    const std::string product = read_content(path + "/product");
    const std::string bMaxPower = read_content(path + "/bMaxPower");

    devices.emplace_back(devnum, idVendor, idProduct, manufacturer, product, bMaxPower);
}