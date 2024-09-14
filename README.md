# PPR 64b - Print Process Registers (64-bit)

**PPR 64b** is a simple command-line tool that allows you to attach to a running 64-bit process on Windows, capture its register state, and display it in real-time. This tool supports both process names and PIDs as input and provides detailed information about the current state of general-purpose registers for a 64-bit process. The tool is capable of continuously updating the register values and detecting changes to individual registers.

## Features

- Attach to a process by name or PID.
- Capture and display general-purpose 64-bit registers.
- Monitor register changes in real-time.
- Supports a customizable refresh interval.
- Graceful exit using `Ctrl+C`.
- Provides warning prompts for potentially resource-intensive or incorrect operations (e.g., continuous refresh or 32-bit process attachment).

## Release
Download the [tool](todo) here.

## Usage

You can run the program by passing either the process name or PID, and the refresh interval (in seconds) as arguments.

```
bash
PPR_64b [process name | PID] [refrash interval]
```

### Example:

Command:
```
bash
PPR_64b notepad.exe 0.3
```

Output:
![Output](https://i.imgur.com/Fr9ldvF.png)