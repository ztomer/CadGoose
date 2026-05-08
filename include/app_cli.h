#ifndef APP_CLI_H
#define APP_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

// Handle command line arguments and execute CLI commands.
// Returns:
//   -1: Start the application normally (e.g. no args or --foreground)
//    0: CLI command executed successfully, application should exit(0)
//    1: CLI command failed, application should exit(1)
int AppCli_HandleCommand(int argc, char** argv, int* appArgc);

#ifdef __cplusplus
}
#endif

#endif // APP_CLI_H