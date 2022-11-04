#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/init_task.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(my_other_get_phy_addr, unsigned long *, initial,
		unsigned long *, result)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	unsigned long padder = 0;
	unsigned long page_addr = 0;
	unsigned long page_offset = 0;
	//printk("Here1");
	unsigned long *vir_adds =
		kmalloc(1 * sizeof(unsigned long),
			GFP_KERNEL); //need to use vmalloc in kernel
	unsigned long *phy_adds =
		kmalloc(1 * sizeof(unsigned long), GFP_KERNEL);

	//copy from user
	//initial need to change type?? ->not need
	//printk("Here2");
	unsigned long a =
		copy_from_user(vir_adds, initial, 1 * sizeof(unsigned long)); //
	printk("%lu", a);

	int i = 0;
	//Input virtual addresses //*(intial+i) //
	

	pgd = pgd_offset(current->mm, *(vir_adds + i));
	printk("pgd_val = 0x%lx\n", pgd_val(*pgd));
	printk("pgd_index = %lu\n", pgd_index(*(vir_adds + i)));
	if (pgd_none(*pgd)) {
		printk("not mapped in pgdn");
	}

	p4d = p4d_offset(pgd, *(vir_adds + i));
	printk("p4d_val = 0x%lx\n", p4d_val(*p4d));
	printk("p4d_index = %lu\n", p4d_index(*(vir_adds + i)));
	if (p4d_none(*p4d)) {
		printk("not mapped in p4d");
	}

	pud = pud_offset(p4d, *(vir_adds + i));
	printk("pud_val = 0x%lx\n", pud_val(*pud));
	printk("pud_index = %lu\n", pud_index(*(vir_adds + i)));
	if (pud_none(*pud)) {
		printk("not mapped in pudn");
	}

	pmd = pmd_offset(pud, *(vir_adds + i));
	printk("pmd_val = 0x%lx\n", pmd_val(*pmd));
	printk("pmd_index = %lu\n", pmd_index(*(vir_adds + i)));
	if (pmd_none(*pmd)) {
		printk("not mapped in pmdn");
	}

	pte = pte_offset_kernel(pmd, *(vir_adds + i));
	printk("pte_val = 0x%lx\n", pte_val(*pte));
	printk("pte_index = %lu\n", pte_index(*(vir_adds + i)));
	if (pte_none(*pte)) {
		printk("not mapped in pten");
	}

	
	page_addr = pte_val(*pte) & PAGE_MASK; 
	page_offset = *(vir_adds + i) & ~PAGE_MASK;

	//physical address
	*(phy_adds + i) = page_addr | page_offset;

	printk("page_addr = %lx\n", page_addr);
	printk("page_offset = %lx\n", page_offset);

	printk("vaddr =%lx, paddr = %lx\n", *(vir_adds + i), *(phy_adds + i));

	//Use copytouser -> user space
	copy_to_user(result, phy_adds, 1 * sizeof(unsigned long));
	kfree(vir_adds);
	kfree(phy_adds);

	return 0;
}