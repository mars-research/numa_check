#include <numa.h>
#include <stdio.h>
#include <stdint.h>

#define _GNU_SOURCE
#define RESET_MASK(x)		~(1LL << (x))

struct node {
	unsigned long cpu_bitmask;
	unsigned int num_cpus;
	uint32_t *cpu_list;
};

struct numa_config {
	int num_nodes;
	int total_cpus;
	struct node *nodes;
};

struct task_placement {
	uint32_t *producer_cpus;
	uint32_t *consumer_cpus;
};

/**
 * These are some preliminary ideas for numa policies. This may not be sufficient to cover all
 * possible cases.
 * Policies:
 * PROD_CONS_SEPARATE_NODES - This mode is to stress the framework as running producers and consumers on
 * different numa nodes
 * PROD_CONS_SAME_NODES - Run producers and consumers on the same numa node. Of course, this is bounded by
 * the number of cpus available on a numa node. prod + cons <= node_x_cpus
 * PROD_CONS_MIXED_MODE - the affinity does not matter. Just run it as long as you can tie this thread to
 * a cpu.
 */
enum numa_policy {
	PROD_CONS_SEPARATE_NODES = 1,
	PROD_CONS_SAME_NODES = 2,
	PROD_CONS_MIXED_MODE = 3,
};

int main()
{
	struct bitmask *cm;
	int num_nodes;
	int n;
	unsigned long cpu_bmap;
	int numa_present;
	int ret = 0;
	struct numa_config *config;

	struct node *nodes;
	// numa_available returns 0 if numa apis are available, else -1
	if ((ret = numa_present = numa_available())) {
		printf("Numa apis unavailable!\n");
		goto err_numa;
	}

	config = calloc(1, sizeof(struct numa_config));

	if (!config) {
		perror("calloc:");
		goto err_numa;
	}

	printf("numa_available: %s\n", numa_present ? "false" : "true");

	printf("numa_max_possible_node: %d\n", numa_max_possible_node());
	printf("numa_num_possible_nodes: %d\n", numa_num_possible_nodes());
	printf("numa_max_node: %d\n", numa_max_node());

	config->num_nodes = num_nodes = numa_num_configured_nodes();
	cm = numa_allocate_cpumask();

	config->nodes = nodes = calloc(num_nodes, sizeof(struct node));

	printf("numa_num_configured_nodes: %d\n", numa_num_configured_nodes());
	printf("numa_num_configured_cpus: %d\n", numa_num_configured_cpus());
	printf("numa_num_possible_cpus: %d\n", numa_num_possible_cpus());

	for (n = 0; n < num_nodes; n++) {
		int num_cpus, cpus = 0;
		if ((ret = numa_node_to_cpus(n, cm))) {
			fprintf(stderr, "bitmask is not long enough\n");
			goto err_range;
		}

		nodes[n].cpu_bitmask = cpu_bmap = *(cm->maskp);
		nodes[n].num_cpus = num_cpus = __builtin_popcountl(cpu_bmap);
		nodes[n].cpu_list = calloc(sizeof(uint32_t), num_cpus);

		// extract all the cpus from the bitmask
		while (cpu_bmap) {
			// cpu number starts from 0, ffs starts from 1.
			unsigned long c = __builtin_ffsll(cpu_bmap) - 1;
			cpu_bmap &= RESET_MASK(c);
			nodes[n].cpu_list[cpus++] = c;
		}
	}

	for (n = 0; n < num_nodes; n++) {
		int cpu;
		printf("Node: %d cpu_bitmask: 0x%08lx | num_cpus: %d\n", n, nodes[n].cpu_bitmask,
						nodes[n].num_cpus);
		for (cpu = 0; cpu < nodes[n].num_cpus; cpu++) {
		       printf("%d ", nodes[n].cpu_list[cpu]);
		}
		printf("\n");
		free(nodes[n].cpu_list);
	}

	free(nodes);

err_range:
	numa_free_cpumask(cm);
err_numa:
	return ret;
}
