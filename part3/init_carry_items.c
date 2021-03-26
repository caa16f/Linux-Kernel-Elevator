#define ENTRY_SIZE 1000
#define PERMS 0644
#define PARENT NULL

static struct file_operations fops;

static char *message;
static int read_p;

struct
{
	int total_count_sheep;
	int total_count_grape;
	int total_count_wolf;
	//int total_count_people;
	struct list_head list;

} carry_items;

#define CARRY_ITEM_SHEEP 0
#define CARRY_ITEM_WOLF 1
#define CARRY_ITEM_GRAPES 2

#define NUM_CARRY_TYPES 3

typedef struct carry_items
{
	int id;
	const char *name;
	struct list_head list;

} Carry_items;

int add_item(int type)
{
	char *name;		//used to assign name
	Carry_items *c; //array of Carried items
	printk(KERN_NOTICE "im add_item\n");
	switch (type)
	{
	case CARRY_ITEM_SHEEP:
		name = "S";
		carry_items.total_count_sheep += 1;
		//carry_items.total_count_people +=1;
		break;
	case CARRY_ITEM_WOLF:
		name = "W";
		carry_items.total_count_wolf += 1;
		//carry_items.total_count_people +=1;
		break;
	case CARRY_ITEM_GRAPES:
		name = "G";
		carry_items.total_count_grape += 1;
		//carry_items.total_count_people +=1;
		break;
	default:
		return -1;
	}

	c = kmalloc(sizeof(Carry_items) * 1, __GFP_RECLAIM);
	if (c == NULL)
		return -ENOMEM;

	c->id = type;
	c->name = name;
	list_add_tail(&c->list, &carry_items.list); /* insert at back of list */

	return 0;
}

int print_items(void)
{
	int i;
	Carry_items *c;
	struct list_head *temp;

	char *buf = kmalloc(sizeof(char) * 100, __GFP_RECLAIM);
	if (buf == NULL)
	{
		printk(KERN_WARNING "print_items\n");
		return -ENOMEM;
	}

	/* init message buffer */
	strcpy(message, "");

	/* headers, print to temporary then append to message buffer */
	sprintf(buf, "Elevator status: %d wolves %d sheep %d grapes \n", carry_items.total_count_wolf, carry_items.total_count_sheep, carry_items.total_count_grape);
	strcat(message, buf);
	//sprintf(buf, "Total length is: %d\n", animals.total_length);   strcat(message, buf);
	//sprintf(buf, "Total weight is: %d\n", animals.total_weight);   strcat(message, buf);
	//sprintf(buf, "Animals seen:\n");                               strcat(message, buf);

	/* print entries */
	i = 0;
	//list_for_each_prev(temp, &animals.list) { /* backwards */
	list_for_each(temp, &carry_items.list)
	{ /* forwards*/
		c = list_entry(temp, Carry_items, list);

		/* newline after every 10 entries */
		if (i % 10 == 0 && i > 0)
			strcat(message, "\n");

		sprintf(buf, "%s ", c->name);
		strcat(message, buf);

		i++;
	}

	/* trailing newline to separate file from commands */
	strcat(message, "\n");

	kfree(buf);
	return 0;
}

void delete_carry_items(int type)
{
	struct list_head move_list;
	struct list_head *temp;
	struct list_head *dummy;
	int i;
	Carry_items *c;

	INIT_LIST_HEAD(&move_list);

	/* move items to a temporary list to illustrate movement */
	//list_for_each_prev_safe(temp, dummy, &animals.list) { /* backwards */
	list_for_each_safe(temp, dummy, &carry_items.list)
	{ /* forwards */
		c = list_entry(temp, Carry_items, list);

		if (c->id == type)
		{
			//list_move(temp, &move_list); /* move to front of list */
			list_move_tail(temp, &move_list); /* move to back of list */
		}
	}

	/* print stats of list to syslog, entry version just as example (not needed here) */
	i = 0;

	//list_for_each_entry_reverse(a, &move_list, list) { /* backwards */
	list_for_each_entry(c, &move_list, list)
	{ /* forwards */
		/* can access a directly e.g. a->id */
		//printk(KERN_NOTICE "im running in delete, i increment\n");
		i++;
	}

	//used to tell how many things were in the classs Carry_items
	printk(KERN_NOTICE "Items carried type %d had %d entries\n", type, i);

	/* free up memory allocation of Carry_items */
	//list_for_each_prev_safe(temp, dummy, &move_list) { /* backwards */
	list_for_each_safe(temp, dummy, &move_list)
	{ /* forwards */
		c = list_entry(temp, Carry_items, list);
		list_del(temp); /* removes entry from list */
		kfree(c);
	}
}

/********************************************************************/

int carry_items_proc_open(struct inode *sp_inode, struct file *sp_file)
{
	read_p = 1;
	printk(KERN_NOTICE "im inside of procs_open\n");
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL)
	{
		printk(KERN_WARNING "carry_items_proc_open\n");
		return -ENOMEM;
	}

	add_item(get_random_int() % NUM_CARRY_TYPES);
	return print_items();
}

ssize_t carry_items_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset)
{
	int len = strlen(message);

	read_p = !read_p;
	if (read_p)
		return 0;

	copy_to_user(buf, message, len);
	return len;
}

int carry_items_proc_release(struct inode *sp_inode, struct file *sp_file)
{
	kfree(message);
	return 0;
}

/********************************************************************/

static int carry_items_init(void)
{
	printk(KERN_NOTICE "carry_init begin\n");
	fops.open = carry_items_proc_open;
	fops.read = carry_items_proc_read;
	fops.release = carry_items_proc_release;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops))
	{
		printk(KERN_WARNING "Problem in init\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}

	carry_items.total_count_sheep = 0;
	carry_items.total_count_grape = 0;
	carry_items.total_count_wolf = 0;
	INIT_LIST_HEAD(&carry_items.list);

	return 0;
}
module_init(carry_items_init);

static void carry_items_exit(void)
{
	delete_carry_items(CARRY_ITEM_GRAPES);
	delete_carry_items(CARRY_ITEM_SHEEP);
	delete_carry_items(CARRY_ITEM_WOLF);
	remove_proc_entry(ENTRY_NAME, NULL);
}
module_exit(carry_items_exit);
