#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

void create_file(char *path, char *content) {
  int fd = open(path, O_CREATE | O_WRONLY);
  if(fd < 0) {
    printf("Ошибка создания файла %s\n", path);
    return;
  }
  write(fd, content, strlen(content));
  close(fd);
  printf("Создан файл %s с содержимым: %s\n", path, content);
}

int check_file_content(char *path, char *expected) {
  int fd = open(path, O_RDONLY);
  if(fd < 0) {
    printf("Не удалось открыть файл %s\n", path);
    return 0;
  }
  
  char buf[128];
  memset(buf, 0, sizeof(buf));
  int n = read(fd, buf, sizeof(buf) - 1);
  close(fd);
  
  if(n < 0) {
    printf("Ошибка чтения из файла %s\n", path);
    return 0;
  }
  
  buf[n] = 0;
  return strcmp(buf, expected) == 0;
}

void print_file_info(char *path) {
  struct stat st;
  
  if(lstat(path, &st) < 0) {
    printf("%s: не удалось получить информацию\n", path);
    return;
  }
  
  printf("%s (тип=%d", path, st.type);
  
  if(st.type == T_SYMLINK) {
    char linkbuf[128] = {0};
    if(readlink(path, linkbuf) >= 0) {
      printf(", указывает на %s", linkbuf);
    }
  }
  
  printf(")\n");
}

int main() {
  printf("Начало тестирования символических ссылок\n");
  
  // 1. Создание файлов и директорий
  printf("\n1. Создание базовых файлов и директорий:\n");
  
  mkdir("/testdir");
  create_file("/testdir/file1.txt", "содержимое файла 1");
  
  mkdir("/testdir/subdir");
  create_file("/testdir/subdir/file2.txt", "содержимое файла 2");
  
  // Создаем более глубокую структуру каталогов для тестирования
  mkdir("/testdir/subdir/subsubdir");
  create_file("/testdir/subdir/subsubdir/file3.txt", "содержимое файла 3");
  
  // 2. Тестирование абсолютных ссылок
  printf("\n2. Тестирование абсолютных ссылок:\n");
  
  if(symlink("/testdir/file1.txt", "/abs_link") < 0) {
    printf("Ошибка создания абсолютной ссылки\n");
  } else {
    printf("Создана абсолютная ссылка: /abs_link -> /testdir/file1.txt\n");
    print_file_info("/abs_link");
    
    printf("Проверка чтения через абсолютную ссылку: ");
    if(check_file_content("/abs_link", "содержимое файла 1")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // 3. Тестирование относительных ссылок
  printf("\n3. Тестирование относительных ссылок:\n");
  
  if(chdir("/testdir") < 0) {
    printf("Ошибка смены каталога на /testdir\n");
  } else {
    if(symlink("file1.txt", "rel_link") < 0) {
      printf("Ошибка создания относительной ссылки\n");
    } else {
      printf("Создана относительная ссылка: rel_link -> file1.txt\n");
      print_file_info("rel_link");
      
      printf("Проверка чтения через относительную ссылку: ");
      if(check_file_content("rel_link", "содержимое файла 1")) {
        printf("УСПЕХ\n");
      } else {
        printf("НЕУДАЧА\n");
      }
      
      // Проверка того, что ссылка работает именно в контексте текущего каталога
      printf("Проверка, что ссылка работает в текущем каталоге: ");
      if(check_file_content("file1.txt", "содержимое файла 1")) {
        printf("файл доступен напрямую - OK\n");
      } else {
        printf("файл недоступен напрямую - ОШИБКА\n");
      }
    }
  }
  
  // Возвращаемся в корневой каталог
  chdir("/");
  
  // 4. Тестирование ссылок на файлы в каталогах выше и ниже
  printf("\n4. Тестирование ссылок на файлы в каталогах выше и ниже:\n");
  
  // 4.1 Ссылка на файл в каталоге ниже (на 1 уровень)
  if(symlink("subdir/file2.txt", "/testdir/down_link") < 0) {
    printf("Ошибка создания ссылки на файл уровнем ниже\n");
  } else {
    printf("Создана ссылка вниз: /testdir/down_link -> subdir/file2.txt\n");
    print_file_info("/testdir/down_link");
    
    printf("Проверка чтения через ссылку вниз: ");
    if(check_file_content("/testdir/down_link", "содержимое файла 2")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // 4.2 Ссылка на файл в каталоге выше (на 1 уровень)
  if(symlink("../file1.txt", "/testdir/subdir/up_link") < 0) {
    printf("Ошибка создания ссылки на файл уровнем выше\n");
  } else {
    printf("Создана ссылка вверх: /testdir/subdir/up_link -> ../file1.txt\n");
    print_file_info("/testdir/subdir/up_link");
    
    printf("Проверка чтения через ссылку вверх: ");
    // Переходим в каталог, где расположена ссылка, чтобы относительный путь работал корректно
    chdir("/testdir/subdir");
    if(check_file_content("up_link", "содержимое файла 1")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА (при вызове из каталога с ссылкой)\n");
      
      // Попробуем еще с абсолютным путем
      if(check_file_content("/testdir/subdir/up_link", "содержимое файла 1")) {
        printf("УСПЕХ (при вызове с абсолютным путем)\n");
      } else {
        printf("НЕУДАЧА (при вызове с абсолютным путем)\n");
      }
    }
  }
  
  // 4.3 Ссылка на файл в каталоге на 2 уровня ниже
  if(symlink("subdir/subsubdir/file3.txt", "/testdir/down2_link") < 0) {
    printf("Ошибка создания ссылки на файл 2 уровнями ниже\n");
  } else {
    printf("Создана ссылка вниз на 2 уровня: /testdir/down2_link -> subdir/subsubdir/file3.txt\n");
    print_file_info("/testdir/down2_link");
    
    printf("Проверка чтения через ссылку вниз на 2 уровня: ");
    chdir("/testdir");
    if(check_file_content("down2_link", "содержимое файла 3")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // 4.4 Ссылка на файл в каталоге на 2 уровня выше
  if(symlink("../../file1.txt", "/testdir/subdir/subsubdir/up2_link") < 0) {
    printf("Ошибка создания ссылки на файл 2 уровнями выше\n");
  } else {
    printf("Создана ссылка вверх на 2 уровня: /testdir/subdir/subsubdir/up2_link -> ../../file1.txt\n");
    print_file_info("/testdir/subdir/subsubdir/up2_link");
    
    printf("Проверка чтения через ссылку вверх на 2 уровня: ");
    chdir("/testdir/subdir/subsubdir");
    if(check_file_content("up2_link", "содержимое файла 1")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // Возвращаемся в корневой каталог
  chdir("/");
  
  // 5. Тестирование нескольких уровней ссылок
  printf("\n5. Тестирование нескольких уровней ссылок:\n");
  
  // 5.1 Абсолютная ссылка на абсолютную ссылку
  if(symlink("/abs_link", "/level1_link") < 0) {
    printf("Ошибка создания ссылки на ссылку (уровень 1)\n");
  } else if(symlink("/level1_link", "/level2_link") < 0) {
    printf("Ошибка создания ссылки на ссылку (уровень 2)\n");
  } else {
    printf("Создана цепочка абсолютных ссылок: /level2_link -> /level1_link -> /abs_link -> /testdir/file1.txt\n");
    
    printf("Проверка чтения через цепочку абсолютных ссылок: ");
    if(check_file_content("/level2_link", "содержимое файла 1")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // 5.2 Абсолютная ссылка на относительную ссылку
  if(symlink("/testdir/rel_link", "/abs_to_rel_link") < 0) {
    printf("Ошибка создания абсолютной ссылки на относительную\n");
  } else {
    printf("Создана абсолютная ссылка на относительную: /abs_to_rel_link -> /testdir/rel_link -> file1.txt\n");
    
    printf("Проверка чтения через абс->отн ссылку: ");
    if(check_file_content("/abs_to_rel_link", "содержимое файла 1")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // 5.3 Относительная ссылка на относительную ссылку
  chdir("/testdir");
  if(symlink("rel_link", "rel_to_rel_link") < 0) {
    printf("Ошибка создания относительной ссылки на относительную\n");
  } else {
    printf("Создана относительная ссылка на относительную: rel_to_rel_link -> rel_link -> file1.txt\n");
    
    printf("Проверка чтения через отн->отн ссылку: ");
    if(check_file_content("rel_to_rel_link", "содержимое файла 1")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  // Возвращаемся в корневой каталог
  chdir("/");
  
  // 6. Тестирование циклических ссылок
  printf("\n6. Тестирование циклических ссылок:\n");
  
  // 6.1 Прямая циклическая ссылка (ссылка на себя)
  if(symlink("/cycle_link", "/cycle_link") < 0) {
    printf("Ошибка создания циклической ссылки\n");
  } else {
    printf("Создана циклическая ссылка: /cycle_link -> /cycle_link\n");
    print_file_info("/cycle_link");
    
    printf("Проверка открытия циклической ссылки (должна быть ошибка): ");
    int fd = open("/cycle_link", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть циклическую ссылку)\n");
      close(fd);
    }
  }
  
  // 6.2 Косвенная циклическая ссылка
  if(symlink("/cycle2", "/cycle1") < 0 || symlink("/cycle1", "/cycle2") < 0) {
    printf("Ошибка создания косвенной циклической ссылки\n");
  } else {
    printf("Создана косвенная циклическая ссылка: /cycle1 -> /cycle2 -> /cycle1\n");
    
    printf("Проверка открытия косвенной циклической ссылки (должна быть ошибка): ");
    int fd = open("/cycle1", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть косвенную циклическую ссылку)\n");
      close(fd);
    }
  }
  
  // 6.3 Более сложная косвенная циклическая ссылка (через 3 перехода)
  if(symlink("/c2", "/c1") < 0 || symlink("/c3", "/c2") < 0 || symlink("/c1", "/c3") < 0) {
    printf("Ошибка создания сложной косвенной циклической ссылки\n");
  } else {
    printf("Создана сложная косвенная циклическая ссылка: /c1 -> /c2 -> /c3 -> /c1\n");
    
    printf("Проверка открытия сложной косвенной циклической ссылки (должна быть ошибка): ");
    int fd = open("/c1", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть сложную косвенную циклическую ссылку)\n");
      close(fd);
    }
  }
  
  // 7. Тестирование ссылок на несуществующие файлы
  printf("\n7. Тестирование ссылок на несуществующие файлы:\n");
  
  // 7.1 Абсолютная ссылка на несуществующий файл
  if(symlink("/nonexistent_file", "/nonexist_link") < 0) {
    printf("Ошибка создания ссылки на несуществующий файл\n");
  } else {
    printf("Создана ссылка на несуществующий файл: /nonexist_link -> /nonexistent_file\n");
    print_file_info("/nonexist_link");
    
    printf("Проверка открытия ссылки на несуществующий файл (должна быть ошибка): ");
    int fd = open("/nonexist_link", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть несуществующий файл)\n");
      close(fd);
    }
  }
  
  // 7.2 Относительная ссылка на несуществующий файл (файл с таким именем существует в другом каталоге)
  create_file("/missing_file", "файл в корне");
  chdir("/testdir");
  if(symlink("missing_file", "missing_link") < 0) {
    printf("Ошибка создания относительной ссылки на несуществующий файл\n");
  } else {
    printf("Создана относительная ссылка на несуществующий файл: missing_link -> missing_file\n");
    print_file_info("missing_link");
    
    printf("Проверка открытия ссылки (должна быть ошибка): ");
    int fd = open("missing_link", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть несуществующий файл)\n");
      close(fd);
    }
    
    printf("Проверка, что файл с таким именем существует в корне: ");
    chdir("/");
    fd = open("missing_file", O_RDONLY);
    if(fd >= 0) {
      close(fd);
      printf("УСПЕХ (файл существует в корне)\n");
    } else {
      printf("НЕУДАЧА (файл не существует в корне)\n");
    }
  }
  
  // 7.3 Относительная ссылка на несуществующий файл в каталоге выше
  chdir("/testdir/subdir");
  if(symlink("../nonexist", "up_miss_link") < 0) {
    printf("Ошибка создания относительной ссылки на несуществующий файл выше\n");
  } else {
    printf("Создана ссылка на несуществующий файл выше: up_miss_link -> ../nonexist\n");
    print_file_info("up_miss_link");
    
    printf("Проверка открытия ссылки (должна быть ошибка): ");
    int fd = open("up_miss_link", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть несуществующий файл)\n");
      close(fd);
    }
  }
  
  // 7.4 Относительная ссылка на несуществующий файл в каталоге ниже
  chdir("/testdir");
  if(symlink("subdir/nonexist", "down_miss_link") < 0) {
    printf("Ошибка создания относительной ссылки на несуществующий файл ниже\n");
  } else {
    printf("Создана ссылка на несуществующий файл ниже: down_miss_link -> subdir/nonexist\n");
    print_file_info("down_miss_link");
    
    printf("Проверка открытия ссылки (должна быть ошибка): ");
    int fd = open("down_miss_link", O_RDONLY);
    if(fd < 0) {
      printf("УСПЕХ (ожидаемая ошибка)\n");
    } else {
      printf("НЕУДАЧА (смог открыть несуществующий файл)\n");
      close(fd);
    }
  }
  
  chdir("/");
  
  // 8. Тестирование работы с ссылками из разных каталогов
  printf("\n8. Тестирование работы с ссылками из разных каталогов:\n");
  
  chdir("/");
  create_file("/testdir/target.txt", "цель в testdir");
  chdir("/testdir");
  if(symlink("target.txt", "context_link") < 0) {
    printf("Ошибка создания ссылки для теста контекста\n");
  } else {
    printf("Создана ссылка в контексте /testdir: /testdir/context_link -> target.txt\n");
    
    // Тест доступа из разных каталогов
    printf("Доступ из /testdir: ");
    chdir("/testdir");
    if(check_file_content("context_link", "цель в testdir")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
    
    printf("Доступ из корня с абсолютным путем: ");
    chdir("/");
    if(check_file_content("/testdir/context_link", "цель в testdir")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
    
    printf("Доступ из подкаталога: ");
    chdir("/testdir/subdir");
    if(check_file_content("../context_link", "цель в testdir")) {
      printf("УСПЕХ\n");
    } else {
      printf("НЕУДАЧА\n");
    }
  }
  
  printf("\nТестирование символических ссылок завершено\n");
  return 0;
}