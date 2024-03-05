CC=gcc


all:
	$(CC) examples/project-2/alloc/src/policiesSet1.c examples/project-2/evict/src/policiesSet1.c impl/usm/usm.c impl/usm/hashmap.c impl/usm/list_sort.c impl/com/com.c impl/com/event.c impl/policies/alloc/alloc.c impl/policies/evict/swap.c examples/project-2/project-2.c -o project-2 -lpthread -DDEBUG

nodebug:
	$(CC) examples/project-2/alloc/src/policiesSet1.c examples/project-2/evict/src/policiesSet1.c impl/usm/usm.c impl/usm/hashmap.c impl/usm/list_sort.c impl/com/com.c impl/com/event.c impl/policies/alloc/alloc.c impl/policies/evict/swap.c examples/project-2/project-2.c -o project-2 -lpthread