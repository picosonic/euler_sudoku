#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/*
https://projecteuler.net/problem=96
  By solving all fifty puzzles find the sum of the 3-digit numbers found in the top left corner of each solution grid
*/

/*
 ' ' = unknown number
 'F' = fixed known number
 'N' = not this number
*/

#define DBG 0
#define NUMRANGE 9

typedef char choice[NUMRANGE];
typedef choice tgrid[9][9];
int changes;
tgrid grid; // Master grid
tgrid *curgrid;
int guesses;
int treedepth;
int optioncountallowed;
long runningtotal;

/* For level zero guesses */
int babyx;
int babyy;
int babynumber;

struct tguess;

typedef struct tguess
{
  int myx;
  int myy;
  int mynumber;

  int babyx;
  int babyy;
  int babynumber;

  tgrid *pgrid;
  struct tguess *myparent;
} guess;

guess *lastguess;
  
/* Completely clear grid to unknowns */
void cleargrid(tgrid *dest)
{
  int x,y,c;

  for (x=0; x<9; x++)
    for (y=0; y<9; y++)
      for (c=0; c<NUMRANGE; c++)
        (*dest)[x][y][c]=' ';
}

/* Copy contents of one grid to another - for guessing */
void copygrid(tgrid *source, tgrid *dest)
{
  int x,y,c;

  for (x=0; x<9; x++)
    for (y=0; y<9; y++)
      for (c=0; c<NUMRANGE; c++)
        (*dest)[x][y][c] = (*source)[x][y][c];
}

/* Print whole grid in debug */
void printdbggrid()
{
  int x,y,c;

  for (y=0; y<9; y++)
  {
    for (x=0; x<9; x++)
    {
      printf("|");
      for (c=0; c<NUMRANGE; c++)
        printf("%c", (*curgrid)[x][y][c]);
    }
    printf("|\n");
  }
}

/* Rule out a whole row for a given number */
int ruleoutrow(int y, int c)
{
  int rowid;
  int changed;
  
  changed=0;
  for (rowid=0; rowid<9; rowid++)
  {
    if ((*curgrid)[rowid][y][c]==' ')
    {
      (*curgrid)[rowid][y][c]='N';
      changed++;
    }
  }

  return changed;
}

/* Rule out a whole column for a given number */
int ruleoutcol(int x, int c)
{
  int colid;
  int changed;
  
  changed=0;
  for (colid=0; colid<9; colid++)
  {
    if ((*curgrid)[x][colid][c]==' ')
    {
      (*curgrid)[x][colid][c]='N';
      changed++;
    }
  }

  return changed;
}

/* Rule out a whole box for a given number */
int ruleoutbox(int x, int y, int c)
{
  int boxx;
  int boxy;
  int xoffs;
  int yoffs;
  int changed;

  changed=0;
  boxx=(x / 3)*3;
  boxy=(y / 3)*3;

  for (yoffs=0; yoffs<3; yoffs++)
    for (xoffs=0; xoffs<3; xoffs++)
    {
      if ((*curgrid)[boxx+xoffs][boxy+yoffs][c]==' ')
      {
        (*curgrid)[boxx+xoffs][boxy+yoffs][c]='N';
        changed++;
      }
    }

  return changed;
}

/* Look for knowns, rule out those numbers from same row/column/box */
void ruleoutgrid()
{
  int x,y,c;
  int changed;

  changed=0;
  for (x=0; x<9; x++)
    for (y=0; y<9; y++)
      for (c=0; c<NUMRANGE; c++)
      {
        if ((*curgrid)[x][y][c]=='F')
        {
          changed+=ruleoutrow(y, c);
          changed+=ruleoutcol(x, c);
          changed+=ruleoutbox(x, y, c);
        }
      }

  changes+=changed;
}

/* Rule out numbers matching c from same row/column/box */
void ruleoutgridxyn(int x, int y, int c)
{
  int changed;

  changed=0;
  if ((*curgrid)[x][y][c]=='F')
  {
    changed+=ruleoutrow(y, c);
    changed+=ruleoutcol(x, c);
    changed+=ruleoutbox(x, y, c);
  }

  changes+=changed;
}

/* Mark the square at x,y as being a certain number */
void markfixed(int x, int y, int number)
{
  int c;

  for (c=0; c<NUMRANGE; c++)
    (*curgrid)[x][y][c]='N';
      
  (*curgrid)[x][y][number]='F';
    
  ruleoutgridxyn(x, y, number);

  if (DBG) printf("FOUND %d,%d is %d\n", x, y, number+1);
}

/* Simply import a 9 char row at a given rowid */
void importrow(int rowid, char *rowstr)
{
  int x;

  if (strlen(rowstr)==9)
  {
    for (x=0; x<9; x++)
    {
      if (rowstr[x]!=' ')
      {
        if ((rowstr[x]>='1') && (rowstr[x]<='9'))
          markfixed(x, rowid, rowstr[x]-'1');
      }
    }
  }
}

/* Return fixed known numbers or ' ' */
char getgrid(int x, int y)
{
  int c;
  
  for (c=0; c<NUMRANGE; c++)
    if ((*curgrid)[x][y][c]=='F') return c+'1';

  return ' ';
}

/* Print grid at currently solved point */
void printgrid()
{
  int x,y;

  for (y=0; y<9; y++)
  {
//    printf("-------------------\n");
    for (x=0; x<9; x++)
    {
  //    printf("|");
      printf("%c", getgrid(x,y));
    }
    printf("\n");
 //   printf("|\n");
  }
//  printf("-------------------\n");
}

/* For a given x,y return number of remaining posibilites */
int optioncount(int x, int y)
{
  int c;
  int counter;
  
  counter=0;
  for (c=0; c<NUMRANGE; c++)
    if ((*curgrid)[x][y][c]==' ') counter++;

  return counter;
}

/* return 1 if a given 3x3 box has a given number fixed */
int hasblock(int boxx, int boxy, int num)
{
  int offsx;
  int offsy;
  int minx;
  int miny;
  int maxx;
  int maxy;

  minx=boxx*3;
  miny=boxy*3;
  maxx=(boxx+1)*3;
  maxy=(boxy+1)*3;

  for (offsy=miny; offsy<maxy; offsy++)
    for (offsx=minx; offsx<maxx; offsx++)
      if ((*curgrid)[offsx][offsy][num]=='F') return 1;

  return 0;
}

/* return 1 if a given number is known in a given column */
int hascol(int x, int num)
{
  int y;

  for (y=0; y<9; y++)
      if ((*curgrid)[x][y][num]=='F') return 1;

  return 0;
}

/* return 1 if a given number is known in a given row */
int hasrow(int y, int num)
{
  int x;
  
  for (x=0; x<9; x++)
      if ((*curgrid)[x][y][num]=='F') return 1;

  return 0;
}

/* return grid column if block contains possible numbers which are only vertical else -1 */
int possnumblockvert(int blockx, int blocky, int number)
{
  int offsx;
  int offsy;
  int lines;
  int got;
  int maxx;
  int maxy;
  int col;

  lines=0;
  maxx=(blockx+1)*3;
  maxy=(blocky+1)*3;

  for (offsx=blockx*3; offsx<maxx; offsx++)
  {
    got=0;
    for (offsy=blocky*3; offsy<maxy; offsy++)
    {
      if ((*curgrid)[offsx][offsy][number]==' ')
      {
        /* remember the line incase its the only one */
        col=offsx;
        got++;
      }
    }

    if (got>0) lines++;
  }

  if (lines==1) return col;

  return -1;
}

/* return grid row if block contains possible numbers which are only horizontal else -1 */
int possnumblockhoriz(int blockx, int blocky, int number)
{
  int offsx;
  int offsy;
  int lines;
  int got;
  int row;
  int maxx;
  int maxy;

  lines=0;
  maxx=(blockx+1)*3;
  maxy=(blocky+1)*3;

  for (offsy=blocky*3; offsy<maxy; offsy++)
  {
    got=0;
    for (offsx=blockx*3; offsx<maxx; offsx++)
    {
      if ((*curgrid)[offsx][offsy][number]==' ')
      {
        /* remember the line incase its the only one */
        row=offsy;
        got++;
      }
    }

    if (got>0) lines++;
  }

  if (lines==1) return row;

  return -1;
}

/* return 1 if a given grid square has any number fixed */
int anynumhere(int x, int y)
{
  int c;

  for (c=0; c<NUMRANGE; c++)
      if ((*curgrid)[x][y][c]=='F') return 1;

  return 0;
}

/* check single grid squares for only one remaining option */
void solvesquare()
{
  int x;
  int y;
  int options;
  int c;
  
  for (y=0; y<9; y++)
  {
    for (x=0; x<9; x++)
    {
      options=optioncount(x, y);

      /* Check for only one possibilty remaining */
      if (options==1)
      {
        for (c=0; c<NUMRANGE; c++)
        {
          if ((*curgrid)[x][y][c]==' ')
            markfixed(x, y, c);
        }
      }
    }
  }
}

/* find vertical single column options for number */
void solvehorizvert()
{
  int number;
  int boxx;
  int boxy;
  int boxx2;
  int col;
  int row;

  for (number=0; number<NUMRANGE; number++)
  {
    boxx=0; boxy=0;

    while (boxy!=3)
    {
      /* Does this box only have single vertical options for this number */
      col=possnumblockvert(boxx, boxy, number);
      if (col!=-1)
      {
        /* Check other blocks on same x axis as this one for horizontal only */
        for (boxx2=0; boxx2<3; boxx2++)
        {
          /* Make sure we dont look in the already found box twice */
          if (boxx2!=boxx)
          {
            row=possnumblockhoriz(boxx2, boxy, number);
            if (row!=-1)
            {
              /* Mask found, vertical one where horizontal is, needs removing */
              if (((*curgrid)[col][row][number]==' ') && (optioncount(col, row)>1))
              {
                (*curgrid)[col][row][number]='N';
                changes++;
              }
            }
          }
        }
      }

      boxx++;
      if (boxx==3)
      {
        boxx=0;
        boxy++;
      }
    }
  }
}

/* find horizontal single row options for number */
void solveverthoriz()
{
  int number;
  int boxx;
  int boxy;
  int boxy2;
  int col;
  int row;

  for (number=0; number<NUMRANGE; number++)
  {
    boxx=0; boxy=0;

    while (boxy!=3)
    {
      /* Does this box only have single horizontal options for this number */
      row=possnumblockhoriz(boxx, boxy, number);
      if (row!=-1)
      {
        /* Check other blocks on same y axis as this one for vertical only */
        for (boxy2=0; boxy2<3; boxy2++)
        {
          /* Make sure we dont look in the already found box twice */
          if (boxy2!=boxy)
          {
            col=possnumblockvert(boxx, boxy2, number);
            if (col!=-1)
            {
              /* Mask found, horizontal one where vertical is, needs removing */
              if (((*curgrid)[col][row][number]==' ') && (optioncount(col, row)>1))
              {
                (*curgrid)[col][row][number]='N';
                changes++;
              }
            }
          }
        }
      }

      boxx++;
      if (boxx==3)
      {
        boxx=0;
        boxy++;
      }
    }
  }
}

/* find horizontal single row options for number and mask boxes on same row */
void solveblockhoriz()
{
  int number;
  int boxx;
  int boxy;
  int boxx2;
  int col;
  int row;

  for (number=0; number<NUMRANGE; number++)
  {
    boxx=0; boxy=0;

    while (boxy!=3)
    {
      /* Does this box only have single horizontal options for this number */
      row=possnumblockhoriz(boxx, boxy, number);
      if (row!=-1)
      {
        /* Mask other blocks on same x axis as */
        for (boxx2=0; boxx2<3; boxx2++)
        {
          /* Make sure we dont look in the already found box twice */
          if (boxx2!=boxx)
          {
            for (col=(boxx2*3); col<((boxx2+1)*3); col++)
            {
              if ((*curgrid)[col][row][number]==' ')
              {
                (*curgrid)[col][row][number]='N';
                changes++;
              }
            }
          }
        }
      }

      boxx++;
      if (boxx==3)
      {
        boxx=0;
        boxy++;
      }
    }
  }
}

/* find vertical single row options for number and mask boxes on same column */
void solveblockvert()
{
  int number;
  int boxx;
  int boxy;
  int boxy2;
  int col;
  int row;

  for (number=0; number<NUMRANGE; number++)
  {
    boxx=0; boxy=0;

    while (boxy!=3)
    {
      /* Does this box only have single vertical options for this number */
      col=possnumblockvert(boxx, boxy, number);
      if (col!=-1)
      {
        /* Mask other blocks on same x axis as */
        for (boxy2=0; boxy2<3; boxy2++)
        {
          /* Make sure we dont look in the already found box twice */
          if (boxy2!=boxy)
          {
            for (row=(boxy2*3); row<((boxy2+1)*3); row++)
            {
              if ((*curgrid)[col][row][number]==' ')
              {
                (*curgrid)[col][row][number]='N';
                changes++;
              }
            }
          }
        }
      }

      boxx++;
      if (boxx==3)
      {
        boxx=0;
        boxy++;
      }
    }
  }
}

/* check grid rows for only one remaining option */
void solverow()
{
  int x;
  int y;
  int c;
  int options;
  int missingnum;
  int missingx;
  
  missingx=-1;
  for (y=0; y<9; y++)
  {
    options=9;

    for (x=0; x<9; x++)
    {
      for (c=0; c<NUMRANGE; c++)
      {
        if ((*curgrid)[x][y][c]=='F')
        {
          options--;
        }
        else
          if ((*curgrid)[x][y][c]==' ')
          {
            missingx=x;
            missingnum=c;
          }
      }
    }
    
    if ((options==1) && (missingx!=-1))
        markfixed(missingx, y, missingnum);
  }
}

/* see if all other positions in a row for a given number
   have been ruled out */
void solveonlyplaceinrow()
{
  int x;
  int y;
  int options;
  int number;
  int foundx;
  int foundy;
  
  for (number=0; number<NUMRANGE; number++)
  {
    for (y=0; y<9; y++)
    {
      options=0;

      for (x=0; x<9; x++)
      {
        if ((*curgrid)[x][y][number]==' ')
        {
          // rember incase its the only one
          foundx=x;
          foundy=y;

          options++;
        }
      }
  
      if (options==1)
        markfixed(foundx, foundy, number);
    }
  }
}

/* check grid columns for only one remaining option */
void solvecol()
{
  int x;
  int y;
  int c;
  int options;
  int missingy;
  int missingnum;

  missingy=-1;
  
  for (x=0; x<9; x++)
  {
    options=9;

    for (y=0; y<9; y++)
    {
      for (c=0; c<NUMRANGE; c++)
      {
        if ((*curgrid)[x][y][c]=='F')
        {
          options--;
        }
        else
          if ((*curgrid)[x][y][c]==' ')
          {
            missingy=y;
            missingnum=c;
          }
      }
    }

    if ((options==1) && (missingy!=-1))
      markfixed(x, missingy, missingnum);
  }
}

/* see if all other positions in a column for a given number
   have been ruled out */
void solveonlyplaceincol()
{
  int x;
  int y;
  int options;
  int number;
  int foundx;
  int foundy;
  
  for (number=0; number<NUMRANGE; number++)
  {
    for (x=0; x<9; x++)
    {
      options=0;

      for (y=0; y<9; y++)
      {
        if ((*curgrid)[x][y][number]==' ')
        {
          // rember incase its the only one
          foundx=x;
          foundy=y;

          options++;
        }
      }
  
      if (options==1)
        markfixed(foundx, foundy, number);
    }
  }
}

/* find only one remaining non-fixed number in a 3x3 box */
void solvebox()
{
  int boxx;
  int boxy;
  int yoffs;
  int xoffs;
  int c;
  int options;
  int missingnum;
  int missingx;
  int missingy;
  
  boxx=0;
  boxy=0;
  missingnum=-1;

  while (boxy!=3)
  {
    options=9;

    for (yoffs=0; yoffs<3; yoffs++)
      for (xoffs=0; xoffs<3; xoffs++)
      {
        for (c=0; c<NUMRANGE; c++)
        {
          if ((*curgrid)[(boxx*3)+xoffs][(boxy*3)+yoffs][c]=='F')
          {
            options--;
          }
          else
            if ((*curgrid)[(boxx*3)+xoffs][(boxy*3)+yoffs][c]==' ')
            {
               /* Remember it incase it is the only one */
               missingx=(boxx*3)+xoffs;
               missingy=(boxy*3)+yoffs;
               missingnum=c;
            }
        }
      }

    if (options==1)
    {
      if (missingnum!=-1)
        markfixed(missingx, missingy, missingnum);
    }

    boxx++;
    if (boxx==3)
    {
      boxx=0;
      boxy++;
    }
  }
}

/* see if all other positions in a box for a given number
   have been ruled out */
void solveonlyplaceinbox()
{
  int boxx;
  int boxy;
  int yoffs;
  int xoffs;
  int options;
  int number;
  int foundx;
  int foundy;
  
  foundx=-1;
  for (number=0; number<NUMRANGE; number++)
  {
    boxx=0;
    boxy=0;
  
    while (boxy!=3)
    {
      options=0;

      for (yoffs=0; yoffs<3; yoffs++)
        for (xoffs=0; xoffs<3; xoffs++)
        {
          if ((*curgrid)[(boxx*3)+xoffs][(boxy*3)+yoffs][number]==' ')
          {
            // remember 1st incase its the only one
            if (options==0)
            {
              foundx=(boxx*3)+xoffs;
              foundy=(boxy*3)+yoffs;
            }
            options++;
          }
        }

      if (options==1)
        if (foundx!=-1) markfixed(foundx, foundy, number);

      boxx++;
      if (boxx==3)
      {
        boxx=0;
        boxy++;
      }
    }
  }
}

/* Look for 2 blocks in same column with same number, see if the position of the remaining number can be inferred */
void solvetripletcol()
{
  int boxx;
  int boxy;
  int numcount;
  int number;
  int col1;
  int col2;
  int nocol;
  int noblock;
  int result;
  int offsx;
  int offsy;
  
  for (number=0; number<NUMRANGE; number++)
  {
    for (boxx=0; boxx<3; boxx++)
    {
      numcount=0;

      for (boxy=0; boxy<3; boxy++)
      {
        result=hasblock(boxx, boxy, number);
        if (result==0) noblock=boxy;
        numcount+=result;
      }

      if (numcount==2)
      {
        if (hascol(boxx*3, number))
        {
          col1=boxx*3;
          if (hascol((boxx*3)+1, number))
          {
            col2=(boxx*3)+1;
            nocol=(boxx*3)+2;
          }
          else
          {
            nocol=(boxx*3)+1;
            col2=(boxx*3)+2;
          }
        }
        else
        {
          nocol=(boxx*3);
          col1=(boxx*3)+1;
          col2=(boxx*3)+2;
        }

        if (DBG)
          printf("%d in 2 blocks in same column %d (%d and %d, but not %d) missing from block %d\n", number+1, boxx, col1, col2, nocol, noblock);
          
        offsx=nocol;
        offsy=noblock*3;

        /* check top */
        if (((*curgrid)[offsx][offsy][number]==' ') &&
            (anynumhere(offsx, offsy+1)) &&
            (anynumhere(offsx, offsy+2))
           )
        {
          markfixed(offsx, offsy, number);
        }
        else
        /* check middle */
        if (((*curgrid)[offsx][offsy+1][number]==' ') &&
            (anynumhere(offsx, offsy)) &&
            (anynumhere(offsx, offsy+2))
           )
        {
          markfixed(offsx, offsy+1, number);
        }
        else
        /* check bottom */
        if (((*curgrid)[offsx][offsy+2][number]==' ') &&
            (anynumhere(offsx, offsy)) &&
            (anynumhere(offsx, offsy+1))
           )
        {
          markfixed(offsx, offsy+2, number);
        }

      }
    }
  }
}

/* Look for 2 blocks in same row with same number, see if the position of the remaining number can be inferred */
void solvetripletrow()
{
  int boxx;
  int boxy;
  int numcount;
  int number;
  int row1;
  int row2;
  int norow;
  int result;
  int noblock;
  int offsx;
  int offsy;
  
  for (number=0; number<NUMRANGE; number++)
  {
    for (boxy=0; boxy<3; boxy++)
    {
      numcount=0;

      for (boxx=0; boxx<3; boxx++)
      {
        result=hasblock(boxx, boxy, number);
        if (result==0) noblock=boxx;
        numcount+=result;
      }

      if (numcount==2)
      {
        if (hasrow(boxy*3, number))
        {
          row1=boxy*3;
          if (hasrow((boxy*3)+1, number))
          {
            row2=(boxy*3)+1;
            norow=(boxy*3)+2;
          }
          else
          {
            norow=(boxy*3)+1;
            row2=(boxy*3)+2;
          }
        }
        else
        {
          norow=(boxy*3);
          row1=(boxy*3)+1;
          row2=(boxy*3)+2;
        }

        if (DBG)
          printf("%d in 2 blocks in same row %d (%d and %d, but not %d) missing from block %d\n", number+1, boxy, row1, row2, norow, noblock);
        offsx=noblock*3;
        offsy=norow;

        /* check top */
        if (((*curgrid)[offsx][offsy][number]==' ') &&
            (anynumhere(offsx+1, offsy)) &&
            (anynumhere(offsx+2, offsy))
           )
        {
          markfixed(offsx, offsy, number);
        }
        else
        /* check middle */
        if (((*curgrid)[offsx+1][offsy][number]==' ') &&
            (anynumhere(offsx, offsy)) &&
            (anynumhere(offsx+2, offsy))
           )
        {
          markfixed(offsx+1, offsy, number);
        }
        else
        /* check bottom */
        if (((*curgrid)[offsx+2][offsy][number]==' ') &&
            (anynumhere(offsx, offsy)) &&
            (anynumhere(offsx+1, offsy))
           )
        {
          markfixed(offsx+2, offsy, number);
        }

      }
    }
  }
}

/* Run all known solving steps */
void solvestep()
{
  solvesquare();
  solverow();
  solvecol();
  solvebox();
  
  solvetripletrow();
  solvetripletcol();

  solveonlyplaceincol();
  solveonlyplaceinbox();

  solvehorizvert();
  solveverthoriz();
  
  solveblockhoriz();
  solveblockvert();
}

/* Return number of invalid squares - no more options */
int exhaustedsquares()
{
  int blank;
  int x;
  int y;

  blank=0;
  for (y=0; y<9; y++)
  {
    for (x=0; x<9; x++)
      if ((optioncount(x, y)==0) && (getgrid(x, y)==' '))
        blank++;
  }

  return blank;
}

/* Print how complete the puzzle is, or return 1 for complete */
int showstatus(int print)
{
  int x;
  int y;
  int blank;
  int number;
  int lines;
  int incomplete;

  incomplete=0;

  /* Look for squares with no more options */
  blank=exhaustedsquares();
  if (blank>0)
  {
    if (print) printf("%d squares cannot be any number\n", blank);
    incomplete++;
  }

  /* Check rows are complete */
  lines=0;
  for (y=0; y<9; y++)
  {
    blank=0;
    for (number=0; number<NUMRANGE; number++)
      if (hasrow(y, number)!=1)
      {
        if ((DBG) && (print)) printf("row %d missing a %d\n", y, number+1);
        blank++;
      }

    if (blank!=0) lines++;
  }
  if (lines>0)
  {
    if (print) printf("%d rows incomplete\n", lines);
    incomplete++;
  }

  /* Check columns are complete */
  lines=0;
  for (x=0; x<9; x++)
  {
    blank=0;
    for (number=0; number<NUMRANGE; number++)
      if (hascol(x, number)!=1)
      {
        if ((DBG) && (print)) printf("column %d missing a %d\n", x, number+1);
        blank++;
      }

    if (blank!=0) lines++;
  }
  if (lines>0)
  {
    if (print) printf("%d columns incomplete\n", lines);
    incomplete++;
  }

  /* Check 3x3's are complete */
  lines=0;
  for (x=0; x<3; x++)
    for (y=0; y<3; y++)
    {
      blank=0;
      for (number=0; number<NUMRANGE; number++)
        if (hasblock(x, y, number)!=1)
        {
          if ((DBG) && (print)) printf("block %d,%d missing a %d\n", x, y, number+1);
          blank++;
        }
      if (blank!=0) lines++;
    }
  if (lines!=0)
  {
    if (print) printf("%d 3x3 blocks incomplete\n", lines);
    incomplete++;
  }


  if (incomplete==0)
  {
    if (print) printf("Puzzle SOLVED\n");
    return 1;
  }
  else
  {
    if (print) printf("%d incomplete criterior, FAILED to solve puzzle\n", incomplete);
    return 0;
  }
}

void makeguess()
{
  int x;
  int y;
  int number;
  guess *newguess;
  int minx;
  int miny;
  int minnum;
  int blank;
  int numoptions;
  
  minx=0; miny=0; minnum=0;
  
  blank=exhaustedsquares();
  if (blank>0)
  {
    if (DBG) printf("Wrong guess, backtrack\n");

    if (lastguess!=NULL)
    {      
      // Remove last one
      changes++;
      treedepth--;
      free(lastguess->pgrid);
      newguess=NULL;
      if (lastguess->myparent!=NULL)
      {
        newguess=lastguess->myparent;
        newguess->babynumber=newguess->babynumber+1;
      }
      else
        babynumber++; // First guess was wrong

      free(lastguess);
      lastguess=newguess;

      if (lastguess!=NULL)
      {
        curgrid=lastguess->pgrid;
      }
      else
        curgrid=&grid;
    }
  }

  if (lastguess!=NULL)
  {
    minx=lastguess->babyx;
    miny=lastguess->babyy;
    minnum=lastguess->babynumber;
  }
  else
  {
    minx=babyx;
    miny=babyy;
    minnum=babynumber;
  }

  if (DBG)
  {
    printf("Making guess...\n");
    printf("minx=%d miny=%d minnum=%d\n", minx, miny, minnum);
  }

  /* Look for a guess we can make */
  for (y=miny; y<9; y++)
  {
    for (x=minx; x<9; x++)
    {
      /* Does this square only have x options (starting at 2) */
      numoptions=optioncount(x, y);
      if (((numoptions>=2) && (numoptions<=optioncountallowed)) && (getgrid(x, y)==' '))
      {
        for (number=minnum; number<NUMRANGE; number++)
        {
          if ((*curgrid)[x][y][number]==' ')
          {
            guesses++;
            if (DBG) printf("Guess (%d) %d,%d is %d at depth %d\n", guesses, x, y, number+1, treedepth);
            
            newguess=malloc(sizeof(guess));
            if (newguess!=NULL)
            {
              newguess->myparent=lastguess;
              newguess->pgrid=NULL;

              newguess->pgrid=malloc(sizeof(tgrid));
              if (newguess->pgrid==NULL)
              {
                printf("Out of memory allocating guess grid\n");
                free(newguess);
                exit(1);
              }
            }
            else
            {
              printf("Out of memory allocating guess\n");
              exit(1);
            }

            treedepth++;
            newguess->babyx=0;
            newguess->babyy=0;
            newguess->babynumber=0;

            newguess->myx=x;
            newguess->myy=y;
            newguess->mynumber=number;
            copygrid(curgrid, newguess->pgrid);
            curgrid=newguess->pgrid;

            markfixed(x, y, number);

            // First guess
            if (lastguess==NULL)
            {
              lastguess=newguess;
              babyx=x;
              babyy=y;
              babynumber=number;
            }
            else
            {
              // Further guess
              newguess->myparent=lastguess;
              lastguess->babyx=x;
              lastguess->babyy=y;
              lastguess->babynumber=number;

              lastguess=newguess;
            }

            return;
          }
        }
      }
      if (minnum!=0) minnum=0;
    }
    if (minx!=0) minx=0;
  }
  
  // Dead end - try another avenue
  if (DBG) printf("LOST at %d deep\n", treedepth);
  if (lastguess!=NULL)
  {
    changes++;
    treedepth--;
    free(lastguess->pgrid);
    newguess=NULL;
    if (lastguess->myparent!=NULL)
    {
      newguess=lastguess->myparent;
      newguess->babynumber=newguess->babynumber+1;
    }
    else
      babynumber++;
      
    free(lastguess);

    lastguess=newguess;

    if (lastguess!=NULL)
    {
      curgrid=lastguess->pgrid;
    }
    else
      curgrid=&grid;
  }
  else
  {
    /* If nothing was found with only x options
       remaining, then increase x by one and try again */
    optioncountallowed++;
    if (optioncountallowed<9)
      changes++;
  }
}

int solvepuzzle()
{
  guess *newguess;
  
  newguess=NULL;
  changes=0;
  lastguess=NULL;
  guesses=0;
  treedepth=0;
  optioncountallowed=2;
  babyx=0;
  babyy=0;
  babynumber=0;
  
  ruleoutgrid(); changes=1;

  while (changes!=0)
  {
    if (DBG) printf("%d changes\n", changes);
    changes=0;

    solvestep();

    /* Out of logic, need to make educated guess now ! */
    if ((changes==0) && (showstatus(0)==0))
        makeguess();
  }

  if (showstatus(0)==0)
    printdbggrid();

  printgrid();

  showstatus(1);

  if (showstatus(0)==1)
    runningtotal+=(((getgrid(0, 0)-'0')*100)+((getgrid(1, 0)-'0')*10)+((getgrid(2, 0)-'0')));
  else
    printf("*** ERROR NOT SOLVED ***\n");

  if (guesses>0)
  {
    printf("%d guesses made enroute\n", guesses);

    // Deallocate guess tree
    if (lastguess!=NULL)
    {
      printf("Correct guesses are :\n");
      while (lastguess!=NULL)
      {
        printf("%d@[%d,%d]=%d  ", treedepth, lastguess->myx, lastguess->myy, lastguess->mynumber+1);
        treedepth--;
        free(lastguess->pgrid);
        newguess=lastguess->myparent;
        free(lastguess);
        lastguess=newguess;
      }
      printf("\n");
    }
  }
  
  return 0;
}

void testpuzzles()
{
  int puzznum;
  int rownum;
  FILE *fp;
  char instr[256];
  int instrlen;

  fp=fopen("p096_sudoku.txt", "r");
  if (fp==NULL) return;

  runningtotal=0;
  puzznum=0;
  rownum=0;
  instrlen=0;
  while (!feof(fp))
  {
    instr[instrlen]=fgetc(fp);
    if (feof(fp)) instr[instrlen]='\n';

    switch (instr[instrlen])
    {
      case '\r':
      case '\n':
        instr[instrlen]=0;

        if (instr[0]=='G') // Start of a new grid definition
        {
          puzznum++;
          curgrid=&grid;
          cleargrid(curgrid);
        }
        else // It's a row of numbers
        {
          importrow(rownum++, instr);

          if (rownum==9) // Attempt a solve
          {
            solvepuzzle();

            rownum=0;
          }
        }

        instrlen=0;
        break;

      case '0':
        instr[instrlen++]=' ';
        break;

      default:
        instrlen++;
    }
  }

  fclose(fp);
}

int main()
{
  testpuzzles();
  printf("TOTAL : %ld\n", runningtotal);
  
  return 0;
}
