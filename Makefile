all:	mandlebrot-sequential mandlebrot-mpi-static

clean:	
	rm *.o sequential parallel-static

mandlebrot-sequential:	mandlebrot_sequential.cpp
	mpic++ -o sequential mandlebrot_sequential.cpp -lboost_system -lpng

mandlebrot-mpi-static:	mandlebrot_mpi_static.cpp
	mpic++ -o parallel-static mandlebrot_mpi_static.cpp -lboost_system -lpng