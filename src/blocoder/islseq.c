
#include "cmod.h"
#include "darray.h"
#include "file.h"
#include "isl.h"
#include "islseq.h"
#include "memory.h"
#include "string.h"

/*--------------------------------------------------------------------------*/

void ISLSeq_Init(ISLSeq *islseq, const ISLSeq *islseq_org) {

	if (islseq_org == NULL) {
		islseq->isl = DArray_Create(16);
		islseq->score = 0;
		islseq->canvas_score = 0;
		islseq->opscore = 0;
	} else {
		islseq->isl = DArray_Clone(islseq_org->isl, (CloneFunc *)ISL_Clone);
		islseq->score = islseq_org->score;
		islseq->canvas_score = islseq_org->canvas_score;
		islseq->opscore = islseq_org->opscore;
	}
}

/*--------------------------------------------------------------------------*/

ISLSeq *ISLSeq_Create(void) {
	ISLSeq *islseq;

	islseq = Memory_Reserve(1, ISLSeq);
	ISLSeq_Init(islseq, NULL);

	return islseq;
}

/*--------------------------------------------------------------------------*/

ISLSeq *ISLSeq_Clone(const ISLSeq *islseq) {
	ISLSeq *islseq2;

	islseq2 = Memory_Reserve(1, ISLSeq);
	ISLSeq_Init(islseq2, islseq);

	return islseq2;
}

/*--------------------------------------------------------------------------*/

void ISLSeq_InvalidateScores(ISLSeq *seq) {
    seq->score = -1;
    seq->canvas_score = -1;
    seq->opscore = -1;
}

/*--------------------------------------------------------------------------*/

void ISLSeq_Append(ISLSeq *dest, const ISLSeq *src) {
    for (int i=0; i<ISLSeq_NofISL(src); i++) {
        ISLSeq_Add(dest, ISL_Clone(ISLSeq_ISL(src, i)));
    }
}

/*--------------------------------------------------------------------------*/

void ISLSeq_ToFile(const ISLSeq *seq, int id) {
    String solfile[128];
    String_Printf(solfile, "%i/%07i.isl", id, seq->score);
    File *f = File_Open(solfile, "w");
    if (f == NULL) {
        fprintf(stderr, "ERROR Cannot open file %s -", solfile);
        String_Printf(solfile, "solution-%02i-%07i.isl", id, seq->score);
        fprintf(stderr, "falling back to %s\n", solfile);
    }
    File_Printf(f, "# Solver : blocoder\n");
    File_Printf(f, "# Problem : %i\n", id);
    if (seq->score >= 0) {
        File_Printf(f, "# SCORE : %i\n", seq->score);
        if (seq->opscore >= 0)
            File_Printf(f, "# Instructions : %i\n", seq->opscore);
        if (seq->canvas_score >= 0)
            File_Printf(f, "# Canvas error : %i\n\n", seq->canvas_score);
    }

    for (int i=0; i<ISLSeq_NofISL(seq); i++) {
        String *instr = ISL_ToString(ISLSeq_ISL(seq, i));
        printf("\t%s\n", instr);
        fprintf(f, "%s\n", instr);
        String_Destroy(instr);
    }
    File_Close(f);
}

/*--------------------------------------------------------------------------*/

void ISLSeq_Del(ISLSeq *seq, int n) {
    DArray_Del(seq->isl, n);
}

/*--------------------------------------------------------------------------*/

void ISLSeq_Print(const ISLSeq *seq) {
    for (int i=0; i<ISLSeq_NofISL(seq); i++) {
        String *instr = ISL_ToString(ISLSeq_ISL(seq, i));
        printf("%s\n", instr);
        String_Destroy(instr);
    }    
}

/*--------------------------------------------------------------------------*/

void ISLSeq_Restore(ISLSeq *islseq) {
	DArray_Destroy(islseq->isl, (DestFunc *)ISL_Destroy);
}

/*--------------------------------------------------------------------------*/

void ISLSeq_Destroy(ISLSeq *islseq) {
	ISLSeq_Restore(islseq);
	Memory_Free(islseq, ISLSeq);
}

/*--------------------------------------------------------------------------*/

