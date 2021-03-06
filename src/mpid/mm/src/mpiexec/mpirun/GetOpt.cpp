/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "GetOpt.h"
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag)
{
	if (flag == NULL)
		return false;

	for (int i=0; i<argc; i++)
	{
		if (_tcsicmp(argv[i], flag) == 0)
		{
			for (int j=i; j<argc; j++)
			{
				argv[j] = argv[j+1];
			}
			argc -= 1;
			return true;
		}
	}
	return false;
}

bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, int *n)
{
	if (flag == NULL)
		return false;

	for (int i=0; i<argc; i++)
	{
		if (_tcsicmp(argv[i], flag) == 0)
		{
			if (i+1 == argc)
				return false;
			*n = _ttoi(argv[i+1]);
			for (int j=i; j<argc-1; j++)
			{
				argv[j] = argv[j+2];
			}
			argc -= 2;
			return true;
		}
	}
	return false;
}

bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, long *n)
{
	int i;
	if (GetOpt(argc, argv, flag, &i))
	{
		*n = (long)i;
		return true;
	}
	return false;
}

bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, double *d)
{
	if (flag == NULL)
		return false;

	for (int i=0; i<argc; i++)
	{
		if (_tcsicmp(argv[i], flag) == 0)
		{
			if (i+1 == argc)
				return false;
			*d = _tcstod(argv[i+1], NULL);
			for (int j=i; j<argc-1; j++)
			{
				argv[j] = argv[j+2];
			}
			argc -= 2;
			return true;
		}
	}
	return false;
}

bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, LPTSTR str)
{
	if (flag == NULL)
		return false;

	for (int i=0; i<argc; i++)
	{
		if (_tcsicmp(argv[i], flag) == 0)
		{
			if (i+1 == argc)
				return false;
			_tcscpy(str, argv[i+1]);
			for (int j=i; j<argc-1; j++)
			{
				argv[j] = argv[j+2];
			}
			argc -= 2;
			return true;
		}
	}
	return false;
}
