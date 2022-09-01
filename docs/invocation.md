# Invocations

Invocation is a piece of metadata like "this user ran this version of this code on this machine with these arguments".

It's useful for:

* understanding what's going on in general (who's running what)
* insights about which solvers produce better results
* troubleshooting, e.g. if a bug in the solver is discovered, we can see which solutions were affected and have to be rerun

Every table probably should have the following column:
```
invocation_id INTEGER NOT NULL REFERENCES invocations_raw(id)
```

Invocations are recorded using `record_this_invocation()`.
You pass it expected time till the next update.
If invocation is not updated by this deadline, it's considered lost (abnormally terminated).
It only affects how it is presented in the invocations table in the dashboard.
```
let invocation_id = record_this_invocation(KeepRunning { seconds: 35 });
sleep(30);
let invocation_id = record_this_invocation(KeepRunning { seconds: 35 });
sleep(30);
let invocation_id = record_this_invocation(Stopped);
```
