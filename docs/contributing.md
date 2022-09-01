No mandatory code reviews. Just don't break the build.

To help with that, you can install pre-push git hook that will run tests:
```
cp git_hooks/pre-push .git/hooks/pre-push
cp git_hooks/pre-push .git/hooks/pre-commit  # if you want to be extra strict
```

Please try to fix or silence all warnings (see [clippy.md](clippy.md)).

The repository will become public after the contest.
Don't commit secrets you can't revoke.
