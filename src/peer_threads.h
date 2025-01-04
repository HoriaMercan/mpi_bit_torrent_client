#ifndef __PEER_THREADS_H__
#define __PEER_THREADS_H__

void *download_thread_func(void *arg);
void *upload_thread_func(void *arg);
void *get_loading_info_thread_func(void *arg);

#endif  /* __PEER_THREADS_H__ */