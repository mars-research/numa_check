#include <numa.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iostream>

#define RESET_MASK(x)		~(1LL << (x))

using namespace std;

struct node {
	unsigned int id;
	unsigned long cpu_bitmask;
	unsigned int num_cpus;
	std::vector<uint32_t> cpu_list;
};

class Numa {
	public:
		Numa() : numa_present(!numa_available())
		{
			if (numa_present) {
				max_node = numa_max_node();
				max_possible_node = numa_max_possible_node();
				num_possible_nodes = numa_num_possible_nodes();
				num_configured_nodes =numa_num_configured_nodes();

				num_configured_cpus =numa_num_configured_cpus();
				num_possible_cpus =numa_num_possible_cpus();
			}
		}

		inline bool isNumaAvailable(void) const {
			return numa_present;
		}

		friend std::ostream& operator<<(std::ostream &os, const Numa &n) {
			printf("NUMA available: %s\n", n.numa_present ? "true" : "false");
			printf("Node config:\n");
			printf("\tnuma_num_possible_nodes: %d\n", n.num_possible_nodes);
			printf("\tnuma_num_configured_nodes: %d\n", n.num_configured_nodes);
			printf("CPU config:\n");
			printf("\tnuma_num_possible_cpus: %d\n", n.num_possible_cpus);
			printf("\tnuma_num_configured_cpus: %d\n", n.num_configured_cpus);
			return os;
		}

		const int get_num_nodes() const {
			return num_configured_nodes;
		}

		void append_node(struct node &node) {
			nodes.push_back(node);
		}

		void print_numa_nodes(void) {
			std::for_each(nodes.begin(), nodes.end(), [](auto &node) {
				printf("Node: %d cpu_bitmask: 0x%08lx | num_cpus: %d\n\t",
						node.id, node.cpu_bitmask, node.num_cpus);
				std::for_each(node.cpu_list.begin(), node.cpu_list.end(), [](uint32_t &cpu) {
							printf("%d ", cpu);
						});
				printf("\n");
			});

		}

	private:
		int numa_present;
		int max_node;
		int max_possible_node;
		int num_possible_nodes;
		int num_configured_nodes;
		int num_configured_cpus;
		int num_possible_cpus;
		int num_nodes;
		std::vector<struct node> nodes;
};

int main()
{
	struct bitmask *cm;
	int num_nodes;
	unsigned long cpu_bmap;
	auto ret = 0;

	Numa _n;

	if (!_n.isNumaAvailable()) {
		printf("Numa apis unavailable!\n");
		goto err_numa;
	}

	num_nodes = _n.get_num_nodes();
	cm = numa_allocate_cpumask();

	std::cout << _n;

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
		_n.append_node(node);
	}
	_n.print_numa_nodes();

err_range:
	numa_free_cpumask(cm);
err_numa:
	return ret;
}
