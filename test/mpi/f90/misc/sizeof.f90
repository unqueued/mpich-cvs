!  
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
          use mpi
          integer ierr, errs
          integer size1, size2
          real    a
          real    d(20)
          double precision b
          complex c
          character ch

          errs = 0
          call mpi_init(ierr)
          call mpi_sizeof( errs, size1, ierr )
          call mpi_type_size( MPI_INTEGER, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "integer size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( a, size1, ierr )
          call mpi_type_size( MPI_REAL, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "real size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( b, size1, ierr )
          call mpi_type_size( MPI_DOUBLE_PRECISION, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "double precision size is ", size2, " sizeof claims ", size1 
          endif

          call mpi_sizeof( c, size1, ierr )
          call mpi_type_size( MPI_COMPLEX, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "complex size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( ch, size1, ierr )
          call mpi_type_size( MPI_CHARACTER, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "character size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( d, size1, ierr )
          call mpi_type_size( MPI_REAL, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "real array size is ", size2, " sizeof claims ", size1
          endif

          if (errs .gt. 0) then
             print *, ' Found ', errs, ' errors'
          else
             print *, ' No Errors'
          endif
          call mpi_finalize(ierr)
          
        end program main
