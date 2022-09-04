#ifndef _islseq_h
#define _islseq_h

#include "cmod.h"
#include "darray.h"

typedef struct {
	DArray *isl;
	int     score;
	int     canvas_score;
	int     opscore;
} ISLSeq;

ISLSeq *ISLSeq_Create();
ISLSeq *ISLSeq_Clone(const ISLSeq *ISLSeq);
#define ISLSeq_NofISL(islseq) (DArray_NofElems((islseq)->isl))
#define ISLSeq_Nof ISLSeq_NofISL
#define ISLSeq_ISL(islseq, i) ((ISL *)DArray_Elem((islseq)->isl, (i)))
#define ISLSeq_AddISL(islseq, i) (DArray_Add((islseq)->isl, (i)))
#define ISLSeq_Add ISLSeq_AddISL
#define ISLSeq_Score(islseq) ((islseq)->score)
#define ISLSeq_SetScore(islseq, s) (((islseq)->score) = (s))
#define ISLSeq_CanvasScore(islseq) ((islseq)->canvas_score)
#define ISLSeq_SetCanvasScore(islseq, c) (((islseq)->canvas_score) = (c))
#define ISLSeq_Opscore(islseq) ((islseq)->opscore)
#define ISLSeq_SetOpscore(islseq, o) (((islseq)->opscore) = (o))
void    ISLSeq_Del(ISLSeq *seq, int n);
void    ISLSeq_InvalidateScores(ISLSeq *seq);
void    ISLSeq_Append(ISLSeq *dest, const ISLSeq *src);
void    ISLSeq_ToFile(const ISLSeq *seq, int id);
void    ISLSeq_Print(const ISLSeq *seq);
void    ISLSeq_Destroy(ISLSeq *islseq);

#ifdef CMOD_NOMAKEFILE
#include "islseq.c"
#endif
#endif

