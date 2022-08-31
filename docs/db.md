# DB

[Shared database](https://www.enterpriseintegrationpatterns.com/patterns/messaging/SharedDataBaseIntegration.html) architecture.

Credentials are stored in source control because!

Schema is documented in `create_tables()` function. Migrations are done manually.

Use append-only style where it makes sense.
For example, instead of `problems` table with `best_solution` column that has to be updated,
prefer `solutions` table that references `problem_id`,
where you can add new solutions without destroying the old ones.
