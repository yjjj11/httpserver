# ==============================================
# 1. 基础配置（定义目标文件、路径、编译器）
# ==============================================
TARGET =main

SRC_DIR = src
BUILD_DIR = build

INC_DIRS = src \
            src/DatabaseConnectionPool \
            src/mymuduo \
            src/mymuduo/API \
            src/mymuduo/http
INCLUDES = $(addprefix -I, $(INC_DIRS))  

# 编译器和编译选项
CXX = g++
CXXFLAGS = -g -std=c++20 -O2 $(INCLUDES)  
LDFLAGS = -lpthread -lmysqlclient -ljsoncpp -lredis++ -lhiredis  

SRCS = $(shell find $(SRC_DIR) -name "*.cpp")

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))


all: $(TARGET)

# 链接规则：把所有.o文件合并成可执行文件
$(TARGET): $(OBJS)
	$(CXX) -g $(OBJS) -o  $@ $(LDFLAGS)  
	@echo "✅ 链接完成：$@"

# 编译规则：把每个.cpp文件编译成.o文件（分模块处理）
# %是通配符，匹配任意路径和文件名
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@) 
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP  
	@echo "🔨 编译模块：$< → $@"


# ==============================================
# 4. 依赖管理（自动处理头文件修改）
# ==============================================
# 生成依赖文件.d（记录.o依赖哪些头文件）
DEPS = $(OBJS:.o=.d)
-include $(DEPS)  # 引入所有.d文件，让Make自动跟踪头文件变化


debug:
	@echo INCLUDES=$(INCLUDES)
    
clean:
	rm -rf $(BUILD_DIR) $(TARGET) 
	@echo "🧹 清理完成"

# 声明伪目标（避免与同名文件冲突）
.PHONY: all clean debug
