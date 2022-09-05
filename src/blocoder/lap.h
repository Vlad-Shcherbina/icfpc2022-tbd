#ifndef _lap_h
#define _lap_h

#include "cmod.h"
//#include "permutation.h"

typedef struct {
	int     num_rows;
	int     num_cols;
   int     real_rows;
   int     real_cols;
   double  max_cost;
	double**cost;
	int   **assignment;
} LAP;

LAP    *LAP_Create(int rows, int cols);
LAP    *LAP_Clone(LAP *lap);
#define LAP_Rows(lap) ((lap)->num_rows)
#define LAP_Cols(lap) ((lap)->num_cols)
#define LAP_NofCols LAP_Cols
#define LAP_NofRows LAP_Rows
#define LAP_Cost(lap, c, r) ((lap)->cost[c][r])
#define LAP_SetCost(lap, c, r, v) (((lap)->cost[c][r]) = (v))
#define LAP_Assignment(lap, i, j) ((lap)->assignment[i][j])
#define LAP_SetAssignment(lap, i, j, a) (((lap)->assignment[i][j]) = (a))
int     LAP_AssignmentForRow(LAP *lap, int row);
int     LAP_AssignmentForColumn(LAP *lap, int col);
#define LAP_SolutionRow LAP_AssignmentForColumn
#define LAP_SolutionColumn LAP_AssignmentForRow
#define LAP_SolutionCol LAP_AssignmentForRow
double  LAP_Maximize(LAP *p);
double  LAP_Minimize(LAP *p);
double  LAP_ApproximateMax(LAP *p);
double  LAP_ApproximateMin(LAP *p);
void    LAP_Destroy(LAP *lap);
void    LAP_Display(LAP *lap, int n);

#ifdef CMOD_NOMAKEFILE
#include "lap.c"
#endif
#endif

