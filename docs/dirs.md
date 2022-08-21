# Data directories

* `data/` is for stuff like example inputs from the problem statement.
  Produced by humans for programs.
  Everything here is under version control.
* `cache/` is for generated files that could be helpful to have around,
  but should be safe to delete at any time
  (it's possible to regenerate them if needed).
  Produced by programs for programs.
  It's in gitignore.
* `outputs/` is for logs, images rendered by visualizers, etc.
  In other word, stuff produced by programs for humans.
  It's in gitignore.

By the way, there is an utility to get paths relative to the project root:
`project_path("data/example.txt")`.
