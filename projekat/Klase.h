#ifndef KLASE_H_INCLUDED
#define KLASE_H_INCLUDED

#include "tbb/task.h"

#define BLACK 0
#define WHITE 255
#define SUCC_COUNT 9
#define SELF_TASK 0

using namespace std;
using namespace tbb;

extern int* OutputBuffer;
extern int brojacSerial;

class WriteTask : public task
{

public:

    WriteTask(int _i, int _j, int _width) :
        i(_i),
        j(_j),
        width(_width)	// potrebno zbog indeksiranja matrice pomocu niza
    {
    }

    task* execute()
    {

        __TBB_ASSERT(ref_count() == 0, NULL);

            if(counter < 3)
            {
                OutputBuffer[j * width + i] = WHITE - OutputBuffer[j * width + i];
            }


            if (successor != NULL)
            {
                successor->decrement_ref_count();
            }

        return NULL;
    }

public:

    int i;
    int j;
    unsigned int width;
    unsigned int counter;
    empty_task* successor;

};


class ReadTask : public task
{

public:

    ReadTask(int _i, int _j, int _width) :
        i(_i),
        j(_j),
        width(_width)
    {
    }

    task* execute()
    {

        __TBB_ASSERT(ref_count() == 0, NULL);

        int br = 0;

                for (int m = -1; m <= 1; m++)
                {
                    for (int n = -1; n <= 1; n++)
                    {
                        // da ne poredimo boju samu sa sobom
                        if (m != 0 || n != 0)
                        {
                            // provera da li okolna tacka ima istu boju kao i tacka poredjenja
                            if (OutputBuffer[(j + n) * width + (i + m)] == OutputBuffer[j * width + i])
                            {
                                    br++;
                            }

                        }
                    }
                }


        for (int k = 0; k < SUCC_COUNT; k++)
        {
            if (successor[k] != NULL)
            {
                if (k == SELF_TASK)
                {
                    successor[k]->counter = br;
                }

                if(successor[k]->decrement_ref_count() == 0)
                {
                    spawn(*successor[k]);
                }
            }
        }

        return NULL;
    }

public:

    int i;
    int j;
    unsigned int width;
    WriteTask* successor[SUCC_COUNT];

};

#endif // KLASE_H_INCLUDED
