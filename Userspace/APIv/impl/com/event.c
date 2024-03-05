#include "../../include/usm/usm.h" 		// toMod..


#ifdef DEBUG
bool hsh_iter(const void *item, void *udata) {
    const struct usm_process_link *user = item;
    printf("%s (pol=%ld)\n", user->procName, user->alloc_pol);  // TODO hmm.. swp_pol...
    return true;
}
int shown=0;
#endif

int usm_handle_events(struct usm_event *evt) {
    int ret = 0, retried = 0;
    switch (evt->type) {
    case ALLOC:
#ifdef DEBUG
        if(evt->flags&UFFD_PAGEFAULT_FLAG_WRITE)
                printf("Write attempt : \t");
        else
                printf("Read attempt : \t");
#endif
        if(unlikely(evt->flags&UFFD_PAGEFAULT_FLAG_SWAP)) {
#ifdef DEBUG
                printf("Sw.A.C\n");
                if(unlikely(!shown)) {
                        getchar();
                        shown++;
                }
#endif
                ret = 1;
                if (!trySwapCache(evt))
                        ret = usmPolicyDefinedSwapIn(evt);
                else {
#ifdef DEBUG
                        printf("Resolved through swap cache!\n");
#endif
                }
                //getchar();
        }
        else {
retry:
#ifdef DEBUG
                printf("F.A.C..\n");
#endif
                ret = usmPolicyDefinedAlloc(evt);
        }
        
        // USM tells them we'd need... things.... through evt #SpatialDistrib...., and if he doesn't..... uhhh, later.
        if (unlikely(ret)) {
            if(unlikely(retried)) {
#ifdef DEBUG
                printf("[Sys] Failed allocation! Aboring.\n");       // OOMB. to replace abortion.
#endif
                abort();
            }
#ifdef DEBUG
            printf("[Sys] Failed allocation! Assuming full memory and trying to swap out once.\n");       // basically full memory, but could be related to a failed submission... #toMod./Complify
#endif
            usm_evict_fallback(evt);               // maybe some return value and no bother going back up if nothing done.. but trying again can't hurt, as something could have been freed meanwhile..
            // evict_fallback.. if good, goto ^, if not, OOM... so boom everything for now or just ignore that process for a while...
            // OOM bhvr code..                      // gotta be really extremely rare.. -_-'.. with a good swapping policy obviously...
            retried++;
            goto retry;
        }
#ifdef DEBUG
        printf("Memory consumed : %.2f%s #freePage\n", usedMemory/1024/1024>0?(float)usedMemory/1024/1024:(float)usedMemory/1024, usedMemory/1024/1024>0?"MB":"KB");    // should be done by dev.?
#endif
        break;
    case PHY_AS_CHANGE:
#ifdef DEBUG
        printf("Received %lu ! #freePage\n", evt->paddr);
#endif
        ret=usmPolicyDefinedPageIndexFree(evt);			// struct page...! 'event.....
        //do_cond_swap_in();
#ifdef DEBUG
        printf("Memory consumed : %.2f%s #freePage\n", usedMemory/1024/1024>0?(float)usedMemory/1024/1024:(float)usedMemory/1024, usedMemory/1024/1024>0?"MB":"KB");    // should be done by dev.?
#endif
        break;
    case FREE:
        //ret = usmPolicyDefinedFree(evt);		// origin'll be compliant..
        break;
    case VIRT_AS_CHANGE:
        // remapping... call swapping code or simply update all relevant fields (e.g. virt. addresses in pages..)
        break;
    case NEW_PROC:
        // assignment strat... +workerChoice..RandifNotDefined(will be treated in chooseWorker)
        usmSetupProcess(evt);
        /*
        	for uffd|usm, there's, for the moment, one process per channel.... quite easily alterable though.
        */
        struct usm_worker * chosenWorker = usmChooseWorker();
        appendUSMChannel(&usm_cnl_userfaultfd_ops,evt->origin,&(chosenWorker->usm_channels));              // inside usmSetupProcess?
#ifdef DEBUG
        printf("\tProcess %d randomly welcomed by %s.\n",evt->origin, chosenWorker->thread_name);
#endif
        // We could wake the process here instead...
        break;
    case PROC_DTH:              /* .. eventually need to hold kernel side ctx.., pure every related event at this' reception, then release it here alongside everything else.. them allocations don't take this all into account until now..*/
        // cleaning..
        struct usm_channels_node * channelNode=list_entry(evt->channelNode, struct usm_channels_node, iulist);
        pthread_mutex_lock(&channelNode->chnmutex);
        list_del(evt->channelNode);
        pthread_mutex_unlock(&channelNode->chnmutex);							// absolutely unsufficient... prev. and next're involved.. freak it man...! But... are they problematic...
        free(channelNode->usm_channel);
        free(channelNode);
        // no need to check all them page table elements... as they'll be received either way!
        resetUsmProcess(evt->origin);					// bound to create some troubles if lock not used....
#ifdef DEBUG
        printf("Process %d just reached the afterworld.\n",evt->origin);
#endif
        break;
    default:
#ifdef DEBUG
        printf("Weird event.. \n");
#endif
        ret=1;
        break;
    }
/*__do_cond_swap_out(evt);              process no tanjobi... ikene yo..sonnamon ^^'... matte.. ikeru -_-'... thiotto okashi kedo thiooto dake ikeru -_-... TODO.
__do_cond_swap_in(evt);       */
    do_cond_swap_in(evt);             /* Hence merge the two..? */
    do_cond_swap_out(evt);
    return ret;
}

int events_compare(void *priv, const struct list_head *a, const struct list_head *b) {
    return usmGetEventPriority(list_entry(a,usm_events_node,iulist)->event)<usmGetEventPriority(list_entry(b,usm_events_node,iulist)->event)?1:0;
}

int usm_sort_events(struct list_head *events) {
    list_sort(NULL,events,events_compare);
}