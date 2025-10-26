# Enable logging to a file
set logging file schedule_trace.log
set logging overwrite on
set logging enabled on

tbreak ROWNOT::execute
run
continue

break schedule
commands
    printf "\n=== schedule() called ===\n"
    printf "Frame info:\n"
    info args
    printf "Locals:\n"
    info locals
    printf "Backtrace:\n"
    backtrace 12
    printf "=========================\n"
    continue
end

# Run automatically
run
