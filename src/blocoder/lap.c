
#include "cmod.h"
#include "lap.h"
#include "memory.h"

#define LAP_NOT_ASSIGNED 0 
#define LAP_ASSIGNED 1

/*--------------------------------------------------------------------------*/

void LAP_Init(LAP *lap, LAP *lap_org) {
   int i,j;
	if (lap_org == NULL) {
      lap->cost = Memory_Reserve(lap->num_rows, double *);
      lap->assignment = Memory_Reserve(lap->num_rows, int *);
      lap->max_cost = 0.0;
      for(i=0; i<lap->num_rows; i++) {
         lap->cost[i] = Memory_Reserve(lap->num_cols, double);
         lap->assignment[i] = Memory_Reserve(lap->num_cols, int);
         for (j=0; j<lap->num_cols; j++) lap->assignment[i][j] = 0;
      }
	} else {
		lap->num_rows = lap_org->num_rows;
		lap->num_cols = lap_org->num_cols;
		lap->cost = lap_org->cost;
		lap->assignment = lap_org->assignment;
	}
}

/*--------------------------------------------------------------------------*/

LAP *LAP_Create(int rows, int cols) {
	LAP *lap;

	lap = Memory_Reserve(1, LAP);

   lap->real_rows = rows;
   lap->real_cols = cols;
/*
   lap->num_rows = rows;
   lap->num_cols = cols;
*/
   lap->num_rows = MAX(rows, cols);
   lap->num_cols = MAX(rows, cols);

	LAP_Init(lap, NULL);

	return lap;
}

/*--------------------------------------------------------------------------*/

LAP *LAP_Clone(LAP *lap) {
	LAP *lap2;

	lap2 = Memory_Reserve(1, LAP);
	LAP_Init(lap2, lap);

	return lap2;
}

/*--------------------------------------------------------------------------*/

double LAP_Maximize(LAP *p) {
   int i,j;

   p->max_cost = p->cost[0][0];
   for(i=0; i<p->num_rows; i++) {
      for(j=0; j<p->num_cols; j++) {
         if (p->max_cost < p->cost[i][j])
            p->max_cost = p->cost[i][j];
      }
   }

   for(i=0; i<p->num_rows; i++) {
      for(j=0; j<p->num_cols; j++) {
      	p->cost[i][j] = p->max_cost - p->cost[i][j];
      }
   }
   return MIN(p->num_rows,p->num_cols)*p->max_cost - LAP_Minimize(p);
}

/*--------------------------------------------------------------------------*/

double LAP_Minimize(LAP *p) {
   int i, j, m, n, k, l, t, q, unmatched;
   double s, cost;
   int *col_mate;
   int *row_mate;
   int *parent_row;
   int *unchosen_row;
   double *row_dec;
   double *col_inc;
   double *slack;
   int* slack_row;

   cost = 0;
   m = p->num_rows;
   n = p->num_cols;

   col_mate = (int*)calloc(p->num_rows,sizeof(int));
   unchosen_row = (int*)calloc(p->num_rows,sizeof(int));
   row_dec  = (double*)calloc(p->num_rows,sizeof(double));
   slack_row  = (int*)calloc(p->num_rows,sizeof(double));
 
   row_mate = (int*)calloc(p->num_cols,sizeof(int));
   parent_row = (int*)calloc(p->num_cols,sizeof(int));
   col_inc = (double*)calloc(p->num_cols,sizeof(double));
   slack = (double*)calloc(p->num_cols,sizeof(double));

   for (i=0;i<p->num_rows;++i)
      for (j=0;j<p->num_cols;++j)
         p->assignment[i][j] = LAP_NOT_ASSIGNED;

   /* Begin subtract column minima in order to start with lots of zeroes 12 */
   for (l=0;l<n;l++) {
      s = p->cost[0][l];
      for (k=1; k<m; k++) {
         if (p->cost[k][l] < s) s = p->cost[k][l];
      }
      cost += s;
      if (s!=0) {
         for (k=0; k<m; k++) p->cost[k][l] -= s;
      }
   }
   /* End subtract column minima in order to start with lots of zeroes 12 */

   /* Begin initial state 16 */
   t = 0;
   for (l=0; l<n; l++) {
      row_mate[l] = -1;
      parent_row[l] = -1;
      col_inc[l] = 0;
      slack[l] = 1e10;
   }
   
   for (k=0;k<m;k++) {
      s = p->cost[k][0];
      for (l=1;l<n;l++) {
         if (p->cost[k][l] < s) {
            s = p->cost[k][l];
         }
      }
      row_dec[k]=s;
      for (l=0;l<n;l++) {
         if (s==p->cost[k][l] && row_mate[l]<0) {
            col_mate[k]=l;
            row_mate[l]=k;
            goto row_done;
         }
      }

      col_mate[k]= -1;
      unchosen_row[t++]=k;
      row_done: ;
   }
   /* End initial state 16 */
 
   // Begin Hungarian algorithm 18
   if (t==0) goto done;
   
   unmatched = t;
   
   while (TRUE) {
      double del;
      q = 0;
      while (TRUE) {
         while (q<t) {
	      // Begin explore node q of the forest 19
		      k = unchosen_row[q];
            s = row_dec[k];
            for (l=0; l<n; l++) {
               /* Hotspot is here ... */
		         if (slack[l]!=0) {
                  del = p->cost[k][l] - s + col_inc[l];
                  if (del < slack[l]) {
               /* ... to here */
                     if (del==0) {
                        if (row_mate[l]<0) goto breakthru;
                        slack[l] = 0.0;
                        parent_row[l]=k;
                        unchosen_row[t++]=row_mate[l];
                     } else {
                        slack[l] = del;
                        slack_row[l] = k;
                     }
                  }
               }
            }
	      // End explore node q of the forest 19
	         q++;
	      }
 
         // Begin introduce a new zero into the matrix 21
         s = 1e10;
         for (l=0; l<n; l++)
	         if (slack[l]!=0 && slack[l]<s) s = slack[l];
          
         for (q=0;q<t;q++) row_dec[unchosen_row[q]]+=s;
       
         for (l=0;l<n;l++) {
            if (slack[l]) {
               slack[l]-=s;
               if (slack[l]==0) {
                  // Begin look at a new zero 22
                  k=slack_row[l];
                  if (row_mate[l]<0) {
                     for (j=l+1;j<n;j++) {
                        if (slack[j] == 0) col_inc[j]+=s;
                     }
                     goto breakthru;
                  } else {
                     parent_row[l]=k;
                     unchosen_row[t++]=row_mate[l];
                  }
                  // End look at a new zero 22
               }
            } else
               col_inc[l]+=s;
         // End introduce a new zero into the matrix 21
         }
      }
breakthru:
   	// Begin update the matching 20
      while (TRUE) {
         j = col_mate[k];
	      col_mate[k] = l;
         row_mate[l] = k;
         if (j<0) break;
         k = parent_row[j];
         l = j;
      }
      
      // End update the matching 20
      if (--unmatched==0) goto done;
        // Begin get ready for another stage 17
      t=0;
      for (l=0;l<n;l++) {
         parent_row[l] = -1;
         slack[l] = 1e10;
      }
      for (k=0;k<m;k++) {
         if (col_mate[k]<0) unchosen_row[t++]=k;
      }
      // End get ready for another stage 17
   }
done:
  // End doublecheck the solution 23
  // End Hungarian algorithm 18

   for (i=0;i<m;++i) {
      p->assignment[i][col_mate[i]] = LAP_ASSIGNED;
      /*TRACE("%d - %d\n", i, col_mate[i]);*/
   }
   for (k=0;k<m;++k) {
      for (l=0;l<n;++l) {
      /*TRACE("%d ",p->cost[k][l]-row_dec[k]+col_inc[l]);*/
         p->cost[k][l] = p->cost[k][l]-row_dec[k]+col_inc[l];
      }
   /*TRACE("\n");*/
   }
   
   for (i=0;i<m;i++) cost += row_dec[i];
   for (i=0;i<n;i++) cost -= col_inc[i];

   free(slack);
   free(col_inc);
   free(parent_row);
   free(row_mate);
   free(slack_row);
   free(row_dec);
   free(unchosen_row);
   free(col_mate);
   return cost;
}

/*--------------------------------------------------------------------------*/

int LAP_AssignmentForRow(LAP *lap, int row) {
   int i;
   for (i=0; i<lap->num_cols; i++) {
      if (lap->assignment[row][i] == LAP_ASSIGNED) return i;
   }
   return -1;
}

/*--------------------------------------------------------------------------*/

int LAP_AssignmentForColumn(LAP *lap, int col) {
   int i;
   for (i=0; i<lap->num_rows; i++) {
      if (lap->assignment[i][col] == LAP_ASSIGNED) return i;
   }
   return -1;
}

/*--------------------------------------------------------------------------*/

double  LAP_ApproximateMax(LAP *p) {
   int i, j;
   double max, rowmax, colmax;
   
   colmax = rowmax = 0.0;
   for (i=0; i<LAP_NofCols(p); i++) {
      max = LAP_Cost(p, i, 0);
      for (j=1; j<LAP_NofRows(p); j++) {
         if (LAP_Cost(p, i, j) > max) max = LAP_Cost(p, i, j);
      }
      colmax += max;
   }
   
   for (j=0; j<LAP_NofRows(p); j++) {
      max = LAP_Cost(p, 0, j);
      for (i=1; i<LAP_NofCols(p); i++) {
         if (LAP_Cost(p, i, j) > max) max = LAP_Cost(p, i, j);
      }
      rowmax += max;
   }
   return MIN(colmax, rowmax);
}

/*--------------------------------------------------------------------------*/
#include "stdfunc.h"

double  LAP_ApproximateMin(LAP *p) {
   Std_Warning("LAP_ApproximateMin : Not implemented\n");
   return 0.0;
}

/*--------------------------------------------------------------------------*/

void LAP_Display(LAP *lap, int n) {
   int i, j;

   for (j=0; j<n+6; j++) printf(" ");
   for (i=0; i<LAP_NofCols(lap); i++) {
      printf("%6i  ", i);
   }
   printf("\n");
   
   for (j=0; j<n; j++) printf(" ");
   for (i=0; i<LAP_NofCols(lap)+1; i++) {
      printf("%8s", "........");
   }
   printf("\n");
   
   for (i=0; i<LAP_NofRows(lap); i++) {
      for (j=0; j<n; j++) printf(" ");
      printf("%3i : ", i);
      for (j=0; j<LAP_NofCols(lap); j++) {
         if (lap->assignment[i][j] == LAP_ASSIGNED) printf(ANSI_YELLOW);
         printf("%6g  ", LAP_Cost(lap, i, j));
         if (lap->assignment[i][j] == LAP_ASSIGNED) printf(ANSI_GRAY);
      }
      printf("\n");
   }
}

/*--------------------------------------------------------------------------*/

void LAP_Restore(LAP *p) {
   int i;
   for(i=0; i<p->num_rows; i++) {
      Memory_Free(p->cost[i], double);
      Memory_Free(p->assignment[i], int);
   }
   Memory_Free(p->cost, double *);
   Memory_Free(p->assignment, int *);
}

/*--------------------------------------------------------------------------*/

void LAP_Destroy(LAP *lap) {
	LAP_Restore(lap);
	Memory_Free(lap, LAP);
}

/*--------------------------------------------------------------------------*/

