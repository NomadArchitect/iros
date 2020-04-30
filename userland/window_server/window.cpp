#include <assert.h>
#include <fcntl.h>
#include <graphics/pixel_buffer.h>
#include <stdio.h>
#include <sys/mman.h>

#include "window.h"

static wid_t get_next_id() {
    static wid_t next_wid = 1;
    return next_wid++;
}

Window::Window(String shm_path, const Rect& rect, String title, int client_id)
    : m_shm_path(shm_path.string()), m_content_rect(rect), m_id(get_next_id()), m_title(title), m_client_id(client_id) {
    m_rect.set_x(rect.x() - 1);
    m_rect.set_y(rect.y() - 22);
    m_rect.set_width(rect.width() + 2);
    m_rect.set_height(rect.height() + 23);

    {
        int fd = shm_open(shm_path.string(), O_RDWR | O_CREAT | O_EXCL, 0666);
        assert(fd != -1);

        size_t len = rect.width() * rect.height() * sizeof(uint32_t);
        ftruncate(fd, len);

        void* memory = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(memory != MAP_FAILED);

        m_front_buffer = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(memory), rect.width(), rect.height());
        m_front_buffer->clear();

        close(fd);
    }
    {
        shm_path[shm_path.size() - 1]++;
        int fd = shm_open(shm_path.string(), O_RDWR | O_CREAT | O_EXCL, 0666);
        assert(fd != -1);

        size_t len = rect.width() * rect.height() * sizeof(uint32_t);
        ftruncate(fd, len);

        void* memory = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(memory != MAP_FAILED);

        m_back_buffer = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(memory), rect.width(), rect.height());
        m_back_buffer->clear();

        close(fd);
    }
}

Window::~Window() {
    if (m_front_buffer && m_back_buffer && m_shm_path) {
        munmap(m_back_buffer->pixels(), m_back_buffer->size_in_bytes());
        shm_unlink(m_shm_path.string());
        m_shm_path[m_shm_path.size() - 1]++;
        munmap(m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
        shm_unlink(m_shm_path.string());
    }
}