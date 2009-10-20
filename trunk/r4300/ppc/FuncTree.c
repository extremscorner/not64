/* FuncTree.c - Handles the BST for functions within a block
 * by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "Recompile.h"

static PowerPC_func_node** _find(PowerPC_func_node** node, unsigned short addr){
	while(*node){
		if(addr < (*node)->function->start_addr)
			node = &(*node)->left;
		else if(addr >= (*node)->function->end_addr &&
				(*node)->function->end_addr != 0)
			node = &(*node)->right;
		else
			break;
	}
	return node;
}

PowerPC_func* find_func(PowerPC_func_node** root, unsigned short addr){
	PowerPC_func_node* node = *_find(root, addr);
	return node ? node->function : NULL;
}

void insert_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	*node = malloc(sizeof(PowerPC_func_node));
	(*node)->function = func;
	(*node)->left = (*node)->right = NULL;
}

void remove_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	PowerPC_func_node* old = *node;
	if(!old) return;
	if(!(*node)->left)
		*node = (*node)->right;
	else if(!(*node)->right)
		*node = (*node)->left;
	else {
		// The node has two children, find the node's predecessor and swap
		PowerPC_func_node** pre;
		for(pre = &(*node)->left; (*pre)->right; pre = &(*pre)->right);
		(*node)->function = (*pre)->function;
		old = *pre;
		*pre = NULL;
	}

	free(old);
}

