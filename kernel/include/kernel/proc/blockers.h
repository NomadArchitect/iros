#ifndef _KERNEL_PROC_BLOCKERS_H
#define _KERNEL_PROC_BLOCKERS_H 1

#include <bits/pid_t.h>
#include <bits/time_t.h>
#include <stdbool.h>
#include <stdint.h>

struct device;
struct inode;
struct socket;
struct task;

enum block_type {
    SLEEP,
    UNTIL_INODE_IS_READABLE,
    UNTIL_PIPE_IS_READABLE,
    UNTIL_SOCKET_IS_CONNECTED,
    UNTIL_INODE_IS_READABLE_OR_TIMEOUT,
    UNTIL_INODE_IS_WRITABLE,
    UNTIL_SOCKET_IS_READABLE,
    UNTIL_SOCKET_IS_READABLE_WITH_TIMEOUT,
    UNTIL_DEVICE_IS_READABLE,
    UNTIL_DEVICE_IS_WRITEABLE,
    UNTIL_DEVICE_IS_READABLE_OR_TIMEOUT,
    SELECT,
    SELECT_TIMEOUT,
    WAITPID
};

struct block_info {
    enum block_type type;
    bool (*should_unblock)(struct block_info *info);
    union {
        struct {
            clockid_t id;
            struct timespec end_time;
        } sleep_info;
        struct {
            struct inode *inode;
        } until_inode_is_readable_info;
        struct {
            struct inode *inode;
        } until_pipe_is_readable_info;
        struct {
            struct socket *socket;
        } until_socket_is_connected_info;
        struct {
            struct timespec end_time;
            struct inode *inode;
        } until_inode_is_readable_or_timeout_info;
        struct {
            struct inode *inode;
        } until_inode_is_writable_info;
        struct {
            struct socket *socket;
        } until_socket_is_readable_info;
        struct {
            struct socket *socket;
            struct timespec end_time;
        } until_socket_is_readable_with_timeout_info;
        struct {
            struct device *device;
        } until_device_is_readable_info;
        struct {
            struct device *device;
        } until_device_is_writeable_info;
        struct {
            struct device *device;
            struct timespec end_time;
        } until_device_is_readable_or_timeout_info;
        struct {
            int nfds;
            uint8_t *readfds;
            uint8_t *writefds;
            uint8_t *exceptfds;
        } select_info;
        struct {
            int nfds;
            uint8_t *readfds;
            uint8_t *writefds;
            uint8_t *exceptfds;
            struct timespec end_time;
        } select_timeout_info;
        struct {
            pid_t pid;
        } waitpid_info;
    } __info;
#define sleep_info                                 __info.sleep_info
#define until_inode_is_readable_info               __info.until_inode_is_readable_info
#define until_pipe_is_readable_info                __info.until_pipe_is_readable_info
#define until_socket_is_connected_info             __info.until_socket_is_connected_info
#define until_inode_is_readable_or_timeout_info    __info.until_inode_is_readable_or_timeout_info
#define until_inode_is_writable_info               __info.until_inode_is_writable_info
#define until_socket_is_readable_info              __info.until_socket_is_readable_info
#define until_socket_is_readable_with_timeout_info __info.until_socket_is_readable_with_timeout_info
#define until_device_is_readable_info              __info.until_device_is_readable_info
#define until_device_is_writeable_info             __info.until_device_is_writeable_info
#define until_device_is_readable_or_timeout_info   __info.until_device_is_readable_or_timeout_info
#define select_info                                __info.select_info
#define select_timeout_info                        __info.select_timeout_info
#define waitpid_info                               __info.waitpid_info
};

__attribute__((warn_unused_result)) int proc_block_sleep(struct task *current, clockid_t clock, struct timespec end_time);
__attribute__((warn_unused_result)) int proc_block_until_inode_is_readable(struct task *current, struct inode *inode);
__attribute__((warn_unused_result)) int proc_block_until_pipe_is_readable(struct task *current, struct inode *inode);
__attribute__((warn_unused_result)) int proc_block_until_socket_is_connected(struct task *current, struct socket *socket);
__attribute__((warn_unused_result)) int proc_block_until_inode_is_readable_or_timeout(struct task *current, struct inode *inode,
                                                                                      struct timespec end_time);
__attribute__((warn_unused_result)) int proc_block_until_inode_is_writable(struct task *current, struct inode *inode);
__attribute__((warn_unused_result)) int proc_block_until_socket_is_readable(struct task *current, struct socket *socket);
__attribute__((warn_unused_result)) int proc_block_until_socket_is_readable_with_timeout(struct task *current, struct socket *socket,
                                                                                         struct timespec end_time);
__attribute__((warn_unused_result)) int proc_block_until_device_is_readable(struct task *current, struct device *device);
__attribute__((warn_unused_result)) int proc_block_until_device_is_writeable(struct task *current, struct device *device);
__attribute__((warn_unused_result)) int proc_block_until_device_is_readable_or_timeout(struct task *current, struct device *device,
                                                                                       struct timespec end_time);

__attribute__((warn_unused_result)) int proc_block_select(struct task *current, int nfds, uint8_t *readfds, uint8_t *writefds,
                                                          uint8_t *exceptfds);
__attribute__((warn_unused_result)) int proc_block_select_timeout(struct task *current, int nfds, uint8_t *readfds, uint8_t *writefds,
                                                                  uint8_t *exceptfds, struct timespec end_time);
__attribute__((warn_unused_result)) int proc_block_waitpid(struct task *current, pid_t pid);

#endif /* _KERNEL_PROC_BLOCKERS_H */
