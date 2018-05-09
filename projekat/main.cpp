#include <iostream>
#include <stdlib.h>
#include "tbb/tick_count.h"
#include "tbb/task.h"
#include "BitmapRawConverter.h"
#include "Klase.h"

#define _DEFAULT_ "output.bmp"
#define ARG_NUM 4
#define BOUNDARY 128
#define BLACK 0
#define WHITE 255
#define SUCC_COUNT 9
#define SELF_TASK 0

using namespace std;
using namespace tbb;

// globalne promenljive, sluzi za rad sa zadacima
int* OutputBuffer;
int brojacSerial;


/* funkcija koja serijski otklanja sum slike, heuristickim algoritmom:
    - ako najmanje 3 okolne tacke imaju istu boju, uzimamo tu boju kao boju tacke
    - u suprotnom uzimamo boju koju ima vecina okolnih tacaka
    - izbegavamo ivicne delove slike, zbog nedostatka parametara za poredjenje */
void serial_heuristic_filter(int* inBuffer, int* outBuffer, int width, int height)
{
    int br = 0;

    for (int i = 1; i < width - 1; i++)
	{
		for (int j = 1; j < height - 1; j++)
		{
		    // iteriramo kroz okolinu tacke
            for (int m = -1; m <= 1; m++)
			{
				for (int n = -1; n <= 1; n++)
				{
				    // da ne poredimo boju samu sa sobom
					if (m != 0 || n != 0) {

                        // provera da li okolna tacka ima istu boju kao i tacka poredjenja
                        if (inBuffer[(j + n) * width + (i + m)] == inBuffer[j * width + i]) {
                                br++;
                        }

                    }
				}
			}

			// ukoliko nema dovoljno istih okolnih boja, onda menjamo boju
			if (br < 3) {
                brojacSerial++;
                outBuffer[j * width + i] = WHITE - inBuffer[j * width + i];
			}
			br = 0;
		}
	}

}



// Osnovna petlja za paralelno izvrsavanje, imamo 2*(width-1)*(height-1) zadataka
// Koristimo globalni bafer (OutputBuffer)
void parallel_heuristic_filter(int width, int height)
{

    /* prazan zadatak, koji ceka da se zadati posao obavi */
    empty_task* empty = new (task::allocate_root()) empty_task();
    empty->set_ref_count((width-2)*(height-2) + 1); //dodatni 1 za cekanje

    // Inicijalizacija zadataka
    WriteTask* write[(width-2)*(height-2)];
    ReadTask* read[(width-2)*(height-2)];


    // SET_WRITE_TASKS
    //========================================================//

    for (int i = 0; i < width - 2; i++)
	{
		for (int j = 0; j < height - 2; j++)
		{

		    write[j*(width-2) + i] = new (task::allocate_root()) WriteTask(i+1, j+1, width);
		    write[j*(width-2) + i]->successor = empty;

		    // Prethodnik = EDGE
		    if ((i == 0 && j == 0) || (i == width - 3 && j == 0) || (i == 0 && j == height - 3) || (i == width - 3 && j == height - 3))
		    {
                write[j*(width - 2) + i]->set_ref_count(4); // 3 okolna Read-a, 1 sopsteni Read
		    }
		    else if (i == 0 || j == 0 || i == width - 3 || j == height - 3) // Prethodnik = FRAME
		    {
                write[j*(width - 2) + i]->set_ref_count(6); // 5 okolnih Read-a, 1 sopsteni Read
		    }
		    else // Prethodnik = INNER
		    {
                write[j*(width - 2) + i]->set_ref_count(9); // 8 okolnih Read-a, 1 sopsteni Read
		    }
		}
	}



	// SET_READ_TASK_SUCC
    //========================================================//

    for (int i = 0; i < width - 2; i++)
	{
		for (int j = 0; j < height - 2; j++)
		{

            /* READ - Svaki ima po 9 succesora, 8 okolnih + 1 svoj (zbog citanja) */

			read[j*(width-2) + i] = new (task::allocate_root()) ReadTask(i+1, j+1, width);
			read[j*(width-2) + i]->successor[SELF_TASK] = write[j*(width-2) + i];

                if (i == 0)
                {
                	read[j*(width-2) + i]->successor[1] = write[j*(width-2) + (i+1)];

	                    if (j == 0) //EDGE
	                    {
	                        read[j*(width-2) + i]->successor[2] = write[(j+1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[3] = write[(j+1)*(width-2) + (i+1)];
	                    }
	                    else if (j == height - 3) //EDGE
	                    {
	                  
	                        read[j*(width-2) + i]->successor[2] = write[(j-1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[3] = write[(j-1)*(width-2) + (i+1)];
	                    }
	                    else //FRAME
	                    {
	                        read[j*(width-2) + i]->successor[2] = write[(j+1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[3] = write[(j-1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[4] = write[(j-1)*(width-2) + (i+1)];
	                        read[j*(width-2) + i]->successor[5] = write[(j+1)*(width-2) + (i+1)];
	                    }

                }
                else if (i == width - 3)
                {
                	read[j*(width-2) + i]->successor[1] = write[j*(width-2) + (i-1)];

	                    if (j == 0) //EDGE
	                    {
	                        read[j*(width-2) + i]->successor[2] = write[(j+1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[3] = write[(j+1)*(width-2) + (i-1)];
	                    }
	                    else if (j == height - 3) //EDGE
	                    {
	                        read[j*(width-2) + i]->successor[2] = write[(j-1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[3] = write[(j-1)*(width-2) + (i-1)];
	                    }
	                    else //FRAME
	                    {
	                        read[j*(width-2) + i]->successor[2] = write[(j-1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[3] = write[(j+1)*(width-2) + i];
	                        read[j*(width-2) + i]->successor[4] = write[(j-1)*(width-2) + (i-1)];
	                        read[j*(width-2) + i]->successor[5] = write[(j+1)*(width-2) + (i-1)];
	                    }
                }
                else if (j == 0 || j == height - 3)
                {
                    read[j*(width-2) + i]->successor[1] = write[j*(width-2) + (i-1)];
                    read[j*(width-2) + i]->successor[2] = write[j*(width-2) + (i+1)];

                    if (j == 0) //FRAME
                    {
                        read[j*(width-2) + i]->successor[3] = write[(j+1)*(width-2) + i];
                        read[j*(width-2) + i]->successor[4] = write[(j+1)*(width-2) + (i-1)];
                        read[j*(width-2) + i]->successor[5] = write[(j+1)*(width-2) + (i+1)];
                    }
	                else if (j == height - 3) //FRAME
	                {
	                    read[j*(width-2) + i]->successor[3] = write[(j-1)*(width-2) + i];
	                    read[j*(width-2) + i]->successor[4] = write[(j-1)*(width-2) + (i-1)];
	                    read[j*(width-2) + i]->successor[5] = write[(j-1)*(width-2) + (i+1)];
	                }
            	}
                else //INNER
                {
                    int k = 0;

                        for (int m = -1; m <= 1; m++)
                        {
                            for (int n = -1; n <= 1; n++)
                           	{
                                if (m || n)
                                {
                                    read[j*(width-2) + i]->successor[++k] = write[(j + n)*(width-2) + (i + m)];
                               	}
                           	}
                       	}
                }
        }
	}


	// SPAWN_READ_TASK
    //========================================================//

	for (int i = 0; i < width - 2; i++)
	{
		for (int j = 0; j < height - 2; j++)
		{
			task::spawn(*read[j*(width-2) + i]);
		}
	}


    /* cekanje praznog zadatka */
    empty->wait_for_all();
    task::destroy(*empty);
}



// Ovde cemo sve vrednosti iznad 128 staviti kao bela boja(255), a vrednosti ispod 128 kao crna boja(0)
void setBlackOrWhite(int* inBuffer, int* outBuffer, int width, int height)
{
    for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
            if (inBuffer[j * width + i] >= BOUNDARY) {  //turn to white
                outBuffer[j * width + i] = WHITE;
            }
            else {  // turn to black
                outBuffer[j * width + i] = BLACK;
            }
		}
	}
}


int main(int argc, char* argv[])
{
    if (argc != ARG_NUM)
	{
		cout << "Error: Program pokrenuti na sledeci nacin:" << endl;
		cout << "argv[0] -> ./uklanjanje_suma" << endl;
		cout << "argv[1] -> input.bmp" << endl;
		cout << "argv[2] -> Serial_output.bmp" << endl;
		cout << "argv[3] -> Parallel_output.bmp" << endl;
		return 0;
	}

	// 
	

    BitmapRawConverter inputFile(argv[1]);      // konvertovana ulazna slika
    unsigned int width = inputFile.getWidth();      // sirina matrice
	unsigned int height = inputFile.getHeight();    // visina matrice
	
	
	brojacSerial = 0;							// inicijalizacija brojaca promenjenih piksela serijski


	BitmapRawConverter OutputFile(width, height);
	BitmapRawConverter SerialInputFile(width, height);  // inicijalizovan uzlazni fajl u formatu piksela
    BitmapRawConverter SerialOutputFile(width, height);  // inicijalizovan izlazni fajl u formatu piksela
    BitmapRawConverter ParallelOutputFile(width, height);  // inicijalizovan izlazni fajl u formatu piksela


    setBlackOrWhite(inputFile.getBuffer(), OutputFile.getBuffer(), width, height);
    setBlackOrWhite(inputFile.getBuffer(), SerialInputFile.getBuffer(), width, height);
    setBlackOrWhite(inputFile.getBuffer(), SerialOutputFile.getBuffer(), width, height);
    setBlackOrWhite(inputFile.getBuffer(), ParallelOutputFile.getBuffer(), width, height);

    // Inicijalizujemo bafere
    OutputBuffer = ParallelOutputFile.getBuffer();


    OutputFile.pixelsToBitmap((char*)_DEFAULT_); // izlazna cisto crno-bela slika sa sumom


    // SERIAL FILTER //
    //==========================================================//
    cout << endl << "Filterring started." << endl;
    tick_count SerialStartTime = tick_count::now();
    serial_heuristic_filter(SerialInputFile.getBuffer(), SerialOutputFile.getBuffer(), width, height);
    tick_count SerialEndTime = tick_count::now();
    cout << endl << "\tSerial filter finished. Took: " << (SerialEndTime - SerialStartTime).seconds()*1000 << "ms." << endl << endl;

    

    //PARALLEL FILTER
    //==========================================================//
    cout << endl << "Filterring started." << endl;
    tick_count ParallelStartTime = tick_count::now();
    parallel_heuristic_filter(width, height);
    tick_count ParallelEndTime = tick_count::now();
    cout << endl << "\tParallel filter finished. Took: " << (ParallelEndTime - ParallelStartTime).seconds()*1000 << "ms." << endl << endl;

	int equal = 1;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			if (SerialOutputFile.getBuffer()[j*width + i] != ParallelOutputFile.getBuffer()[j*width + i]) {
				equal = 0;
			}
		}
	}
	
	if (equal) {
		cout << "PICTURES MATCH!" << endl;
	}
	else {
		cout << "PICTURES DON'T MATCH!" << endl;
	}

	cout << "PIKSELS_CHANGED = " << brojacSerial << endl;

	SerialOutputFile.pixelsToBitmap(argv[2]); // izlazna (sa otklonjenim sumom) slika u formatu .bmp
    ParallelOutputFile.pixelsToBitmap(argv[3]);  // izlazna (sa otklonjenim sumom) slika u formatu .bmp

    return 0;
}
