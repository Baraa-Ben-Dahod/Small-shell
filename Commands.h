// Ver: 04-11-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <limits.h>     // For PATH_MAX
#include <stdio.h>
#include <unordered_set>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
class SmallShell;
enum State {stopped, running};

class Command {
protected:
    const std::string m_cmd_line;
    char* m_cmd_args[COMMAND_MAX_ARGS + 1];
    int m_num_args;

public:
    explicit Command(const char* cmd_line, bool parse_args = true);
    virtual ~Command();
    virtual void execute() = 0;

    const std::string& getCmdLine() const;
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char *cmd_line, bool parse_args = true);
    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
    static const char WILDCARDS[];
    char m_removed_background_cmd_line[COMMAND_MAX_LENGTH + 1]{};
    bool is_complex_command() const;
public:
    explicit ExternalCommand(const char *cmd_line);
    virtual ~ExternalCommand() = default;
    void execute() override;
};


class RedirectionCommand : public Command {
    std::string m_redirection_char;
    std::string m_command_line;
    std::string m_output_file;
    char m_removed_background_cmd_line[COMMAND_MAX_LENGTH + 1]{};
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() = default;

    void execute() override;
};


class PipeCommand : public Command {
    std::string m_pipe_char;
    std::string m_command_line1;
    std::string m_command_line2;
    char m_removed_background_command1[COMMAND_MAX_LENGTH + 1]{};
    char m_removed_background_command2[COMMAND_MAX_LENGTH + 1]{};
public:
    explicit PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() = default;

    void execute() override;
};


class DiskUsageCommand : public Command {
public:
    explicit DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() = default;

    void execute() override;
};


class WhoAmICommand : public Command {
public:
    explicit WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() = default;

    void execute() override;
};


class USBInfoCommand : public Command {
public:
    explicit USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand()  = default;

    void execute() override;
};


class ChangeDirCommand : public BuiltInCommand {
    std::string m_targetPath;
    char** m_plastPwd;
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char *cmd_line);
    virtual ~GetCurrDirCommand() = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char *cmd_line);
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
    SmallShell* const m_sshell;
    std::string m_prompt;
public:
    ChangePromptCommand(const char *cmd_line, SmallShell *smash);
    virtual ~ChangePromptCommand() = default;
    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    bool m_is_kill_specified = false;
    JobsList* const m_jobs_list;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() = default;

    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        int m_jobId;
        pid_t m_pid;
        std::string m_cmdLine;
        State m_currentState;

    public:
        JobEntry(int job_id, pid_t pid, State curr_state, const Command *cmd);
        JobEntry(int job_id, pid_t pid, State curr_state, std::string  cmd_line);
        ~JobEntry() = default;
        int getJobID() const;
        pid_t getJobPID() const;
        const std::string& getCmdLine() const;
    };

private:
    std::vector<JobEntry> m_jobs;

public:
    JobsList() = default;

    ~JobsList() = default;

    void addJob(const Command* cmd, pid_t pid, bool isStopped = false);
    void addJob(const std::string& cmdLine, pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId) const;
    void removeJobById(int jobId);
    bool empty() const;
    int getMaxJobId() const;
};

class JobsCommand : public BuiltInCommand {
    JobsList* const m_jobsList;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() = default;

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* const m_jobsList;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() = default;

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* const m_jobsList;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() = default;

    void execute() override;
};

class AliasCommand : public BuiltInCommand {
    std::string m_name;
    std::string m_command;
    void extractNameAndCommand();
    bool isValidName() const;
    bool isLegalName() const;
    void insertNewAlias(std::vector<std::pair<std::string, std::string>>& allAlias) const;
    static const std::unordered_set<std::string> RESERVED_KEYWORDS;
public:
    explicit AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() = default;

    void execute() override;

};

class UnAliasCommand : public BuiltInCommand {
public:
    explicit UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() = default;

    void execute() override;
};


class UnSetEnvCommand : public BuiltInCommand {
public:
    explicit UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() = default;

    void execute() override;
};


class SysInfoCommand : public BuiltInCommand {
public:
    explicit SysInfoCommand(const char *cmd_line);

    virtual ~SysInfoCommand() = default;

    void execute() override;
};


class SmallShell {
private:
    SmallShell();
    std::string m_prompt;
    char *m_lastPwd;
    std::vector<std::pair<std::string, std::string>> m_aliases;
    std::string m_real_cmd_line;
    std::string resolveAlias(const char *cmd_line) const;
    JobsList m_jobsList;

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    void setPrompt(const std::string& newPrompt);
    void showPrompt() const;
    static std::vector<std::pair<std::string, std::string>>& getAliases();
    JobsList& getJobsList();
};

#endif //SMASH_COMMAND_H_
