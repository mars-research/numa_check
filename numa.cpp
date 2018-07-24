#include <numa.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>

#define RESET_MASK(x)		~(1LL << (x))

using namespace std;

struct node {
	unsigned int id;
	unsigned long cpu_bitmask;
	unsigned int num_cpus;
	std::vector<uint32_t> cpu_list;
};

int main()
{
	struct bitmask *cm;
	int num_nodes;
	unsigned long cpu_bmap;
	int numa_present;
	auto ret = 0;
	std::vector<struct node> nodes;

	// numa_available returns 0 if numa apis are available, else -1
	if ((ret = numa_present = numa_available())) {
		printf("Numa apis unavailable!\n");
		goto err_numa;
	}

	printf("numa_available: %s\n", numa_present ? "false" : "true");

	printf("numa_max_possible_node: %d\n", numa_max_possible_node());
	printf("numa_num_possible_nodes: %d\n", numa_num_possible_nodes());
	printf("numa_max_node: %d\n", numa_max_node());

	num_nodes = numa_num_configured_nodes();
	cm = numa_allocate_cpumask();

	printf("numa_num_configured_nodes: %d\n", num_nodes);
	printf("numa_num_configured_cpus: %d\n", numa_num_configured_cpus());
	printf("numa_num_possible_cpus: %d\n", numa_num_possible_cpus());

	for (auto n = 0; n < num_nodes; n++) {
		struct node node;
		if ((ret = numa_node_to_cpus(n, cm))) {
			fprintf(stderr, "bitmask is not long enough\n");
			goto err_range;
		}

		node.cpu_bitmask = cpu_bmap = *(cm->maskp);
		node.num_cpus = __builtin_popcountl(cpu_bmap);
		node.id = n;

		// extract all the cpus from the bitmask
		while (cpu_bmap) {
			// cpu number starts from 0, ffs starts from 1.
			unsigned long c = __builtin_ffsll(cpu_bmap) - 1;
			cpu_bmap &= RESET_MASK(c);
			node.cpu_list.push_back(c);
		}
		nodes.push_back(node);
	}

	std::for_each(nodes.begin(), nodes.end(), [](auto &node) {
		printf("Node: %d cpu_bitmask: 0x%08lx | num_cpus: %d\n",
				node.id, node.cpu_bitmask, node.num_cpus);
		std::for_each(node.cpu_list.begin(), node.cpu_list.end(), [](uint32_t &cpu) {
					printf("%d ", cpu);
				});
		printf("\n");
	});


err_range:
	numa_free_cpumask(cm);
err_numa:
	return ret;
}
