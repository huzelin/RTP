#include "common/monitor/monitor_status_impl.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "common/common_defines.h"
#include "common/logging.h"
#include "common/string_utils.h"

namespace common{

using namespace std;

#define RAW_MMAP_LIST_MAGIC 0x11223345
#define COUNTER_PREFIX "C#"
#define TIMER_COUNTER_PREFIX "T#"
#define BINARY_COUNTER_PREFIX "B#"
#define MAX_CACHE_SIZE 10

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

MonitorStatus * MonitorStatus::instance_ = NULL;

class MonitorStatusImpl : public MonitorStatus {
  // raw memory-map list is used to store monitor status
  class RawMMapList{
   public:
    struct ListHeader {
      uint32_t magic;         // magic for monitor file
      uint32_t count;         // entry count (monitor unit count)
      uint32_t max_size;      // max memory size
      uint32_t size;          // size
      uint32_t flag;          // flag
      pthread_mutex_t mutex;  // deprecated
      
      //
      // @param max_file_size maximum size of raw memory map list
      //
      void Init(uint32_t max_file_size) {
        if (flag && magic == RAW_MMAP_LIST_MAGIC) {
          return;
        }
        memset(this, '\0', max_file_size);  // clear
        magic    = RAW_MMAP_LIST_MAGIC;
        flag     = true;
        size     = 0;
        max_size = max_file_size;
        count    = 0;
        // init mutex, deprecated
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, 1);
        pthread_mutexattr_setrobust_np(&attr, 1);
        pthread_mutex_init(&mutex, &attr);
        pthread_mutexattr_destroy(&attr);
      }
    };
    
    // entry is a monitor counter unit
    struct Entry {
      uint32_t total_size;
      uint32_t key_size;
      uint32_t value_size;
      void* Key() { return reinterpret_cast<uint8_t *>(this) + sizeof(Entry); }
      void* Value() { return reinterpret_cast<uint8_t *>(this) + sizeof(Entry) + key_size; }
    };

    // RawMMapList contruction method
    // @param file_name
    // @param max_size
    RawMMapList(const char * file_name, uint32_t max_size) {
      CHECK(file_name != NULL);
      mode_t mode = umask(0);
      int fd = open(file_name, O_RDWR | O_CREAT, 0666);
      umask(mode);
      if (fd == -1) {
        fprintf(stderr, "open file error:%s, %s\n", strerror(errno), file_name);
        return;
      }
      CHECK(fd != -1);
      flock(fd, LOCK_EX);
      struct stat st;
      fstat(fd, &st);
      if (st.st_size == 0) {
        ftruncate(fd, max_size);
        st.st_size = max_size;
      }
      base_ = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE , MAP_POPULATE | MAP_SHARED, fd, 0);
      if (base_  == MAP_FAILED) {
        fprintf(stderr, "mmap file error:%s, %s\n", strerror(errno), file_name);
        flock(fd, LOCK_UN);
        return;
      }
      CHECK(base_ != MAP_FAILED);
      list_header_ = reinterpret_cast<ListHeader *>(base_);
      list_header_->Init(st.st_size);
      flock(fd, LOCK_UN);
      close(fd);
      entrys_ = static_cast<uint8_t *>(base_) + AlignN(sizeof(*list_header_), sizeof(uint64_t));
      end_    = static_cast<uint8_t *>(base_) + st.st_size;
    }
    
    virtual ~RawMMapList() {
      munmap(base_, static_cast<uint8_t *>(end_) - static_cast<uint8_t *>(base_));
      base_   = NULL;
      entrys_ = NULL;
      end_    = NULL;
    }
    
    // add new monitor counter entry
    // @param key entry key address
    // @param key_size entry key size
    // @param value entry value address
    // @param value_size entry value size
    Entry* AddEntry(const void *key, uint32_t key_size, const void *value, uint32_t value_size) {
      CHECK(key != NULL && key_size > 0 && value != NULL && value_size > 0);
      //pthread_mutex_lock(&list_header_->mutex); //lock
      Entry *tmp = Find(key, key_size);
      if (tmp != NULL) {
        //pthread_mutex_unlock(&list_header_->mutex); //unlock
        return tmp;
      }
      Entry * cur = Next(NULL);
      while (cur->key_size != 0 && cur->total_size != 0) { //move to end
        cur = Next(cur);
      }
      cur->key_size   = AlignN(key_size, sizeof(uint64_t));
      cur->value_size = AlignN(value_size, sizeof(uint64_t));

      // copy to file
      memcpy(cur->Key(), key, key_size);
      memcpy(cur->Value(), value, value_size);

      cur->total_size     = cur->key_size  + cur->value_size + sizeof(Entry);
      list_header_->size += cur->total_size;
      CHECK(list_header_->size <= list_header_->max_size);

      ++list_header_->count;
      //pthread_mutex_unlock(&list_header_->mutex); //unlock
      return cur;
    }
    
    // find monitor counter entry according key key_size pair
    // @param key entry key address
    // @param key_size entry key size
    Entry * Find(const void *key, uint32_t key_size) {
      CHECK(key != NULL && key_size > 0);
      
      uint32_t key_buff_size = AlignN(key_size, sizeof(uint64_t));
      void *key_buff = calloc(1, key_buff_size);
      memcpy(key_buff, key, key_size);
      Entry * cur = Next(NULL);
      
      while (cur->key_size != 0 && cur->total_size != 0) {
        if ((cur->key_size == key_buff_size) && (memcmp(cur->Key(), key_buff, key_buff_size) == 0)) {
          break;
        }
        cur = Next(cur);
      }
      free(key_buff);
      return cur->key_size == 0 ? NULL : cur;
    }
    
    // get next entry
    Entry * Next(Entry *from) {
      CHECK(from < end_);
      if (from == NULL) {
        return static_cast<Entry *>(entrys_);
      }
      uint8_t* next = reinterpret_cast<uint8_t *>(from) + from->total_size;
      return reinterpret_cast<Entry *>(next);
    }
    
    // P operation
    void SemP() {
      struct sembuf sem_buf;
      sem_buf.sem_num = 0;
      sem_buf.sem_op = -1;
      sem_buf.sem_flg = SEM_UNDO;
      while (semop(sem_id_, &sem_buf, 1) == -1 && errno == EINTR) ;
    }
    
    // V operation
    void SemV() {
      struct sembuf sem_buf;
      sem_buf.sem_num = 0;
      sem_buf.sem_op = 1;
      sem_buf.sem_flg = SEM_UNDO;
      while (semop(sem_id_, &sem_buf, 1) == -1 && errno == EINTR) ;
    }
    
    // set sem id
    void SetSemId(int sem_id) {
      sem_id_ = sem_id;
    }
    
   private:
    ListHeader *list_header_;
    void *base_;
    void *entrys_;
    void *end_;
    int sem_id_;
  };
 
 public:
  MonitorStatusImpl() : status_list_(NULL) { }
  ~MonitorStatusImpl() { }
  
  // initialization of MonitorStatusImpl
  bool Init(const std::string &monitor_stats_file) {
    status_list_ = new RawMMapList(monitor_stats_file.c_str(), 64*1024);
    key_t key    = ftok(monitor_stats_file.c_str(), 0x66);
    int sem_id   = semget(key, 1, 0666);
    if (sem_id < 0) {
      sem_id = semget(key, 1, 0666 | IPC_CREAT);
      union semun sem_union;
      static unsigned short array[1] = {1};
      sem_union.array = array;
      sem_union.val = 1;
      if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
        return false;
      }
    }
    status_list_->SetSemId(sem_id);
    counter_cache_map_index_ = 0;
    timer_cache_map_index_   = 0;
    binary_cache_map_index_  = 0;
    return true;
  }

  // get monitor counter
  //  key: COUNTER_PREFIX module_name # counter_key
  // @param module_name module name
  // @param counter_key counter key
  virtual volatile uint64_t *GetCounter(const char * module_name, const char * counter_key) {
    CHECK(module_name != NULL);
    CHECK(counter_key != NULL);
    
    char buff[128] = "";
    snprintf(buff, sizeof(buff) - 1, "%s%s#%s", COUNTER_PREFIX, module_name, counter_key);
    string key = buff;

    int current_index = counter_cache_map_index_;
    std::map<std::string, uint64_t *>::iterator mit = counter_cache_map_[current_index].find(key);
    if (mit != counter_cache_map_[current_index].end()) {
      return mit->second;
    }

    uint64_t value = 0;
    CHECK(status_list_ != NULL && "must be Init first");
    // P process for muti-process enviroment
    status_list_->SemP();
    RawMMapList::Entry * entry = status_list_->AddEntry(key.c_str(), key.size() + 1, (void *)(&value), sizeof(value));
    volatile uint64_t * counter = const_cast<volatile uint64_t *>(static_cast<uint64_t *>(entry->Value()));
    
    if (counter_cache_map_[counter_cache_map_index_].find(key) == counter_cache_map_[counter_cache_map_index_].end()) {
      int new_index                       = (counter_cache_map_index_ + 1) % MAX_CACHE_SIZE;
      counter_cache_map_[new_index]       = counter_cache_map_[counter_cache_map_index_]; 
      counter_cache_map_[new_index][key]  = const_cast<uint64_t *>(counter);
      counter_cache_map_index_            = new_index;
    }
    
    status_list_->SemV();
    return counter;
  }
  
  // get timer counter
  //  key: TIMER_COUNTER_PREFIX module_name # timer_key
  // @param module_name module name
  // @param timer_key timer key
  virtual volatile TimerMonitorStatus *GetTimerCounter(const char * module_name, const char * timer_key) {
    CHECK(module_name != NULL);
    CHECK(timer_key != NULL);
    
    char buff[128] = "";
    snprintf(buff, sizeof(buff) - 1, "%s%s#%s", TIMER_COUNTER_PREFIX, module_name, timer_key);
    string key = buff;
    
    int current_index = timer_cache_map_index_;
    std::map<std::string, TimerMonitorStatus *>::iterator mit = timer_cache_map_[current_index].find(key);
    if (mit != timer_cache_map_[current_index].end()) {
      return mit->second;
    }

    TimerMonitorStatus value;
    memset(&value, '\0', sizeof(value));
    CHECK(status_list_ != NULL && "must be Init first");
    // P process for multi-process enviroment
    status_list_->SemP();
    RawMMapList::Entry * entry = status_list_->AddEntry(key.c_str(), key.size() + 1, (void *)(&value), sizeof(value));
    volatile TimerMonitorStatus *timer = const_cast<volatile TimerMonitorStatus *>(static_cast<TimerMonitorStatus *>(entry->Value()));
    
    if (timer_cache_map_[timer_cache_map_index_].find(key) == timer_cache_map_[timer_cache_map_index_].end()) {
      int new_index                    = (timer_cache_map_index_ + 1) % MAX_CACHE_SIZE;
      timer_cache_map_[new_index]      = timer_cache_map_[timer_cache_map_index_];
      timer_cache_map_[new_index][key] = const_cast<TimerMonitorStatus *>(timer);
      timer_cache_map_index_           = new_index;
    }

    status_list_->SemV();
    return timer;
  }

  // get binary counter
  //  key: BINARY_COUNTER_PREFIX module_name # binary_key
  // @param module_name module name
  // @param binary_key binary key
  virtual volatile BinaryMonitorStatus *GetBinaryCounter(const char * module_name, const char * binary_key) {
    CHECK(module_name != NULL);
    CHECK(binary_key != NULL);
    
    char buff[128] = "";
    snprintf(buff, sizeof(buff) - 1, "%s%s#%s", BINARY_COUNTER_PREFIX, module_name, binary_key);
    string key = buff;

    int current_index = binary_cache_map_index_;
    std::map<std::string, BinaryMonitorStatus *>::iterator mit = binary_cache_map_[current_index].find(key);
    if (mit != binary_cache_map_[current_index].end()) {
      return mit->second;
    }

    BinaryMonitorStatus value;
    memset(&value, '\0', sizeof(value));
    CHECK(status_list_ != NULL && "must be Init first");
    
    // V process for multi-enviroment
    status_list_->SemP();
    RawMMapList::Entry * entry = status_list_->AddEntry(key.c_str(), key.size() + 1, (void *)(&value), sizeof(value));
    volatile BinaryMonitorStatus *binary = const_cast<volatile BinaryMonitorStatus *>(static_cast<BinaryMonitorStatus *>(entry->Value()));

    if (binary_cache_map_[binary_cache_map_index_].find(key) == binary_cache_map_[binary_cache_map_index_].end()) {
      int new_index                     = (binary_cache_map_index_ + 1) % MAX_CACHE_SIZE;
      binary_cache_map_[new_index]      = binary_cache_map_[binary_cache_map_index_];
      binary_cache_map_[new_index][key] = const_cast<BinaryMonitorStatus *>(binary);
      binary_cache_map_index_           = new_index;
    }
    
    status_list_->SemV();
    return binary;
  }

  virtual void GetAllCounters(std::map<std::string, uint64_t> *all_counters) {
    assert(all_counters != NULL);
    ParseCounters();
    std::swap(*all_counters, counter_map_);
  }

  virtual void GetAllTimerCounters(std::map<std::string, TimerMonitorStatus> * all_timer_counter) {
    assert(all_timer_counter != NULL);
    ParseCounters();
    std::swap(*all_timer_counter, timer_counter_map_);
  }

  virtual void GetAllBinaryCounters(std::map<std::string, BinaryMonitorStatus> * all_binary_counter) {
    assert(all_binary_counter != NULL);
    ParseCounters();
    std::swap(*all_binary_counter, binary_counter_map_);
  }

 private:
  void ParseCounters() {
    counter_map_.clear();
    timer_counter_map_.clear();
    RawMMapList::Entry * cur = NULL;
    while ((cur = status_list_->Next(cur)) != NULL) {
      if (cur->key_size == 0 || cur->total_size == 0) {
        break;
      }
      std::string key(static_cast<char *>(cur->Key()));
      if (common::StringUtils::startsWith(key.c_str(), COUNTER_PREFIX)) {
        volatile uint64_t * value = const_cast<volatile uint64_t *>(static_cast<uint64_t *>(cur->Value()));
        counter_map_.insert(std::make_pair(key.substr(2), *value));
      } else if (common::StringUtils::startsWith(key.c_str(), TIMER_COUNTER_PREFIX)) {
        TimerMonitorStatus * value = (static_cast<TimerMonitorStatus *>(cur->Value()));
        timer_counter_map_.insert(std::make_pair(key.substr(2), *value));
      } else if (common::StringUtils::startsWith(key.c_str(), BINARY_COUNTER_PREFIX)) {
        BinaryMonitorStatus * value = (static_cast<BinaryMonitorStatus *>(cur->Value()));
        binary_counter_map_.insert(std::make_pair(key.substr(2), *value));
      } else {
        fprintf(stderr, "not support counter type");
        abort();
      }
    }
  }
 
 private:
  std::string monitor_stats_file_;
  RawMMapList * status_list_;
  
  std::map<std::string, uint64_t> counter_map_;
  std::map<std::string, uint64_t *> counter_cache_map_[MAX_CACHE_SIZE];
  volatile int counter_cache_map_index_;
  
  std::map<std::string, TimerMonitorStatus> timer_counter_map_;
  std::map<std::string, TimerMonitorStatus *> timer_cache_map_[MAX_CACHE_SIZE];
  volatile int timer_cache_map_index_;

  std::map<std::string, BinaryMonitorStatus> binary_counter_map_;
  std::map<std::string, BinaryMonitorStatus *> binary_cache_map_[MAX_CACHE_SIZE];
  volatile int binary_cache_map_index_;
  
  time_t time_;
};

MonitorStatus *MonitorStatus::Instance() {
  if (instance_ == NULL) {
    instance_ = new MonitorStatusImpl();
  }
  return instance_;
}

void MonitorStatus::SetGlobalInstance(common::MonitorStatus* instance) {
  instance_ = instance;
}

} // namespace common
