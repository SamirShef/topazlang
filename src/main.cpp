/**
 * @file main.cpp
 *
 * @brief Compiler entry point
 */

#include "../include/semantic/semantic.hpp"
#include "../include/codegen/codegen.hpp"
#include "../include/parser/parser.hpp"
#include "../include/lexer/lexer.hpp"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>

int main(int argc, const char *argv[]) {
    bool print_tokens = false;
    bool print_ir = false;
    bool output_is_object = false;

    if (argc < 2) {
        std::cerr << "\033[33mUsage: topazc \"path/to/src.tp\"\033[0m\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "\033[31mCompilation error: Error openning file: does not exist!\033[0m\n";
        return 1;
    }
    
    std::filesystem::path file_path = std::filesystem::absolute(argv[1]);
    std::string executable_path = file_path;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) {
            print_tokens = true;
        }
        else if (strcmp(argv[i], "--ir") == 0) {
            print_ir = true;
        }
        else if (strcmp(argv[i], "--obj") == 0) {
            output_is_object = true;
        }
        else if (strcmp(argv[i], "--path") == 0) {
            if (i == argc - 1) {
                std::cerr << "\033[31mCompilation error: After \033[0m'--path'\033[31m option should be followed by the path to the output file!\033[0m\n";
                return 1;
            }
            executable_path = argv[++i];
        }
    }

    if (executable_path.find('.') != std::string::npos) {
        for (int i = executable_path.size() - 1; executable_path[i] != '.'; i--) {
            executable_path.pop_back();
        }
        executable_path.pop_back();
    }

    #if defined(_WIN32)
    const char *obj_ext = ".obj";
    const char *exe_ext = ".exe";
    #else
    const char *obj_ext = ".o";
    const char *exe_ext = "";
    #endif
    
    #if defined(_WIN32)
    executable_path += exe_ext;
    #endif
    const std::string object_path = executable_path + obj_ext;

    std::string content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Lexer lexer(content, file_path.string());
    std::vector<Token> tokens = lexer.tokenize();
    if (print_tokens) {
        std::cout << "\033[1m\033[32mTokens:\033[0m\n";
        for (Token& token : tokens) {
            std::cout << token.to_str() << '\n';
        }
    }

    Parser parser(tokens);
    std::vector<AST::StmtPtr> stmts_for_semantic = parser.parse();

    SemanticAnalyzer semantic(stmts_for_semantic, file_path.string());
    semantic.analyze();
    
    parser.reset();
    std::vector<AST::StmtPtr> stmts_for_codegen = parser.parse();
    
    CodeGenerator codegen(stmts_for_codegen, file_path.string());
    codegen.generate();
    if (print_ir) {
        if (print_tokens) {
            std::cout << '\n';
        }
        std::cout << "\033[1m\033[32mLLVM IR:\033[0m\n";
        codegen.print_ir();
    }
    
    std::unique_ptr<llvm::Module> module = codegen.get_module();

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    if (module->getFunction("main") == nullptr) {
        std::cerr << "\033[31mCompilation error: Program does not have entry point 'main'\033[0m" << '\n';
        return 1;
    }
    auto getTriple = []() -> std::string {
        const char *env_triple = std::getenv("TOPAZ_TRIPLE");
        if (env_triple && *env_triple) {
            return std::string(env_triple);
        }

        std::array<char, 256> buffer{};
        std::string result;
        #if defined(_WIN32)
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(_popen("clang -dumpmachine", "r"), _pclose);
        #else
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen("clang -dumpmachine", "r"), pclose);
        #endif
        if (!pipe) {
            return "";
        }
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    };
    auto defaultTripleForHost = []() -> std::string {
        #if defined(_WIN32)
            #if defined(__aarch64__) || defined(_M_ARM64)
                return "aarch64-pc-windows-msvc";
            #else
                return "x86_64-pc-windows-msvc";
            #endif
        #elif defined(__APPLE__)
            #if defined(__aarch64__)
                return "arm64-apple-darwin";
            #else
                return "x86_64-apple-darwin";
            #endif
        #else
            #if defined(__aarch64__)
                return "aarch64-unknown-linux-gnu";
            #else
                return "x86_64-pc-linux-gnu";
            #endif
        #endif
    };
    std::string target_triple = getTriple();
    if (target_triple.empty()) {
        target_triple = defaultTripleForHost();
    }
    module->setTargetTriple(llvm::Triple(target_triple));

    std::string error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        std::cerr << error << '\n';
        return 1;
    }

    std::string CPU = "generic";
    std::string features = "";
    llvm::TargetOptions opt;
    auto reloc_model = std::optional<llvm::Reloc::Model>();
    std::unique_ptr<llvm::TargetMachine> target_machine(target->createTargetMachine(target_triple, CPU, features, opt, reloc_model));
    if (!target_machine) {
        std::cerr << "\033[31mCompilation error: Failed to create TargetMachine for triple '" << target_triple << "'\033[0m\n";
        return 1;
    }

    module->setDataLayout(target_machine->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(object_path, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "\033[31mCompilation error: Could not open file '" << object_path << "': " << ec.message() << "\033[0m\n";
        return 1;
    }

    llvm::legacy::PassManager pass;
    
    auto fileType = static_cast<llvm::CodeGenFileType>(1); // 1 = Object file
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "\033[31mCompilation error: TargetMachine can't emit a file of this type\033[0m\n";
        return 1;
    }

    pass.run(*module);
    dest.flush();
    dest.close();

    if (output_is_object) {
        std::cout << "COMPILING SUCCESS. Built object: " << object_path << '\n';
        return 0;
    }

    const char *env_linker = std::getenv("TOPAZC_LINKER");
    std::string linker = env_linker ? std::string(env_linker) : std::string("clang");
    #if defined(_WIN32)
    std::string link_cmd = linker + std::string(" ") + std::string("\"") + object_path + std::string("\"") + " -o " + std::string("\"") + executable_path + std::string("\"") + " -fuse-ld=lld";
    #elif defined(__APPLE__)
    std::string link_cmd = linker + std::string(" ") + std::string("\"") + object_path + std::string("\"") + " -o " + std::string("\"") + executable_path + std::string("\"");
    #else
    std::string link_cmd = linker + std::string(" ") + std::string("\"") + object_path + std::string("\"") + " -o " + std::string("\"") + executable_path + std::string("\"") + " -no-pie";
    #endif
    auto runAndCapture = [](const std::string& cmd) -> std::pair<int, std::string> {
        std::string output;
        #if defined(_WIN32)
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(_popen(cmd.c_str(), "r"), _pclose);
        #else
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
        #endif
        if (!pipe) {
            return { -1, std::string("Failed to spawn: ") + cmd };
        }
        std::array<char, 512> buf{};
        while (fgets(buf.data(), static_cast<int>(buf.size()), pipe.get()) != nullptr) {
            output += buf.data();
        }
        int code = 0;
        #if defined(_WIN32)
        code = _pclose(pipe.release());
        #else
        code = pclose(pipe.release());
        #endif
        return { code, output };
    };

    auto [linkRes, link_out] = runAndCapture(link_cmd);
    if (linkRes != 0) {
        std::cerr << "\033[31mCompilation error: Link command: " << link_cmd << '\n';
        std::cerr << link_out << '\n';
        std::cerr << "Linking failed with code " << linkRes << "\033[0m\n";
        return 1;
    }

    std::cout << "COMPILING SUCCESS. Built executable: " << executable_path << '\n';
    
    if (std::remove(object_path.c_str()) != 0) {
        std::cerr << "\033[31mCompilation error: Warning: Failed to remove object file: " << object_path << "\033[0m\n";
        return 1;
    }
    
    return 0;
}