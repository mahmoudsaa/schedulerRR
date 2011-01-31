/*
*  Copyright (C) 2010 Simone Gaiarin <simgunz@gmail.com>
*
*  This file is part of SchedulerRR.
*
*  SchedulerRR is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  SchedulerRR is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with SchedulerRR.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "schedulerRR.h"

#include <sstream>
#include <algorithm>


SchedulerRR::SchedulerRR(Processor &p, float timeslice, float duration): proc(p), T(timeslice), D(duration), U(0), jobID(0), taskID(0){}

int SchedulerRR::loadTask(Task &t, bool periodic)
{
    Job j;
    stringstream ss;

    if(!periodic)
    {
        int id = jobID;
        ss << "T" << taskID++ << "-";
        string taskLabel = ss.str();
        for (int i = 0; i < t.size(); i++)
        {
            stringstream ssc(taskLabel);
            ssc.seekp(0,ios::end);
            ssc << id;
            proc.rowLabel(id++,ssc.str());
        }
    }


    for (int i = 0; i < t.size(); i++)
    {
        j = t.getJob(i);
        j.setID(jobID++);

        //Controllo che la deadline sia maggiore del release time
        if((j.getDeadline() == -1) || (j.getReleaseTime() < j.getDeadline()))
        {
            waiting.push(j);

            if(j.getDeadline() != -1)
            {
                proc.print(DEADLINE,j.getID(),j.getDeadline());
                proc.setMaxDeadline(j.getDeadline());
            }
        }
        else
        {
            string bad = "_Job_bad_formatted";
            proc.print(TEXTOVER,j.getID(),-1,bad);
        }
    }
    return 0;
}

int SchedulerRR::loadTask(PeriodicTask &t) //Il task t è polimorfico
{
    int id = jobID;
    stringstream ss;
    ss << "EOP" << taskID;

    //Job non valido
    if(t.getPeriod() == 0)
    {
        return 1;
    }


    float u = t.getExecTime() / t.getPeriod();

    //Total utilization grater then 1: system overload
    if (u + U > 1)
        return 2;

    U += u;

    for (int q = 0; q < D; q+=t.getPeriod())
    {
        vector<Job> newJobs;
        for (int i=0; i < t.size(); i++)
        {
            Job j = t.getJob(i);
            float dead = t.getPeriod()+q;
            if(j.getDeadline() != -1)
                dead = min(dead,j.getDeadline()+q);

            Job nw(j.getReleaseTime()+q,dead,j.getExecTime(),j.getPriority());
            newJobs.push_back(nw);
        }
        Task newTask(newJobs);

        loadTask(newTask,true);

        jobID-=t.size();

        proc.print(VLINE,-1,t.getPeriod()+q,ss.str());
    }

    jobID+=t.size();

    ss.seekp(0,ios::beg);
    ss << "PT" << taskID++ << "-";
    string taskLabel = ss.str();
    for (int i = 0; i < t.size(); i++)
    {
        stringstream ssc(taskLabel);
        ssc.seekp(0,ios::end);
        ssc << id;
        proc.rowLabel(id++,ssc.str());
    }

    return 0;
}

Job SchedulerRR::popJob()
{
   Job j = ready.front();
   ready.pop_front();
   return j;
}

void SchedulerRR::enqueueJob(Job& j)
{
    list<Job>::reverse_iterator rit;
    list<Job>::iterator it;

    rit = ready.rbegin();
    while(rit != ready.rend() && j.getPriority() > rit->getPriority())
        rit++;

    it = rit.base();

    ready.insert(it,j);
}

void SchedulerRR::schedule()
{
    Job  r,*currentJob = NULL;
    int sliceEl = 0;
    int end = -1;


    while(!waiting.empty() || !ready.empty() || !proc.idle())
    {
        //Controlla se ci sono processi Ready e li accoda
        vector<Job> vct;


        while(!waiting.empty() && (r = waiting.top()).getReleaseTime() == proc.getClock())
        {
            vct.push_back(r);
            waiting.pop();
        }

        if(!vct.empty())
            sort(vct.begin(),vct.end());

        for (int i = 0; i < vct.size(); i++)
        {
            enqueueJob(vct[i]);
        }

            //Fine della timeslice o processorre idle
        if(sliceEl == 0)
        {
            if(!proc.idle())
            {
                //Forza il preempt del processore visto che la time slice è finita
                proc.preempt();

                if(!end)
                    enqueueJob(*currentJob);

                currentJob = NULL;
            }

            if(!ready.empty())
            {
                sliceEl = T;
                string failed("_Failed");
                currentJob = &(popJob());
                bool d = false, s = false;
                while(currentJob->getDeadline() != -1 && ( ( d = ( currentJob->getDeadline() <= proc.getClock() ) )  || ( s = ( ( currentJob->getDeadline() - proc.getClock() ) < ( currentJob->getExecTime() - currentJob->getElapsedTime() ) ) ) ) )
                {
                    if(d)
                    {
                        proc.print(READYE,currentJob->getID(),currentJob->getDeadline());
                        proc.print(TEXTOVER,currentJob->getID(),currentJob->getDeadline(),failed);
                    }
                    else if(s)
                    {
                        proc.print(READYE,currentJob->getID(),proc.getClock());
                        proc.print(TEXTOVER,currentJob->getID(),proc.getClock(),failed);
                    }

                    d = s = false;

                    if(!ready.empty())
                        currentJob = &(popJob());
                    else
                    {
                        currentJob = NULL;
                        sliceEl = 0;
                        break;
                    }

                }
            }
        }

        for (int i = 0; i < vct.size(); i++)
        {
            proc.print(READYB,vct[i].getID());
            proc.print(START,vct[i].getID());
        }

        //Eseguo un passo del processore e decremento il tempo di slice corrente solo se il processore non è idle
        end = proc.execute(currentJob);
        if (currentJob != NULL)
            sliceEl--;

        //Se il processo termina prima della fine della timeslice libero il processore
        if(end)
        {
            sliceEl = 0;
            currentJob = NULL;
        }
    }
    proc.filePrint();
}
