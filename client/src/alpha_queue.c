/******************************************************************************
 * Copyright (c) 2010 Joey Degges
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

#include "alpha_queue.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


struct node
{
	void *item;
  struct node *next;
};

struct alpha_queue
{
  struct node *head;
  struct node *tail;
};


struct alpha_queue *
alpha_queue_new (void)
{
  struct alpha_queue *queue = calloc (1, sizeof *queue);
  struct node *node = calloc (1, sizeof *node);

  if (NULL == queue || NULL == node)
    return NULL;

  queue->head = node;
  queue->tail = node;

  return queue;
}

void
alpha_queue_free (struct alpha_queue *queue)
{
  if (queue)
    {
      while (queue->head)
        {
          struct node *next = queue->head->next;
          free (queue->head);
          queue->head = next;
        }
    }

  free (queue);
}

bool
alpha_queue_push (struct alpha_queue *queue, void *item)
{
  struct node *new;

  if (NULL == queue)
    return false;

	new = calloc (1, sizeof *new);
	if (new == NULL)
		return false;

  new->item = item;
  queue->tail->next = new;
  queue->tail = new;

	return true;
}

void *
alpha_queue_pop (struct alpha_queue *queue)
{
  struct node *temp;
  void *item;

  if (NULL == queue)
    return NULL;

  if (NULL == queue->head->next)
    return NULL;

  item = queue->head->next->item;
  temp = queue->head;
  queue->head = queue->head->next;

  free (temp);

  return item;
}
