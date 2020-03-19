#include <iostream>
#include <memory>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DeclGroup.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Mangle.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Basic/Version.h>

#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/ASTContext.h"

#include <llvm/Support/Path.h>
#include <llvm/ADT/StringSwitch.h>

struct ASTVisitor : clang::RecursiveASTVisitor<ASTVisitor> {
	ASTVisitor(clang::SourceManager &sm) : sm(sm) {}
	bool VisitFunctionDecl(clang::FunctionDecl *d) {
		clang::SourceLocation sl = d->getBeginLoc();
		std::cout << "decl: " << d->getQualifiedNameAsString() << ": " << sl.printToString(sm) << std::endl;
		return true;
	}

	bool VisitCallExpr(clang::CallExpr *e) {
		auto callee = e->getDirectCallee();
		if (callee != nullptr) {
			std::cout << "\t--> " << callee->getQualifiedNameAsString() << std::endl;
		}

		return true;
	}

private:
	clang::SourceManager &sm;
};

class ASTConsumer : public clang::ASTConsumer {
public:
	ASTConsumer(clang::CompilerInstance &ci) : ci(ci) {
		ci.getPreprocessor().enableIncrementalProcessing();
	}

	virtual ~ASTConsumer() {
		ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer, true);
	}

	virtual void Initialize(clang::ASTContext& Ctx) override {
		//ci.getDiagnostics().setClient(new BrowserDiagnosticClient(annotator), true);
		ci.getDiagnostics().setErrorLimit(0);

		//std::cout << "Initialize" << std::endl;
	}

	virtual bool HandleTopLevelDecl(clang::DeclGroupRef D) override {
		//std::cout << "HandleTopLevelDecl" << std::endl;
		if (ci.getDiagnostics().hasFatalErrorOccurred()) {
			std::cout << "fatal error occured" << std::endl;
			// Reset errors: (Hack to ignore the fatal errors.)
			ci.getDiagnostics().Reset();
			// When there was fatal error, processing the warnings may cause crashes
			ci.getDiagnostics().setIgnoreAllWarnings(true);
		}

		return true;
	}

	virtual void HandleTranslationUnit(clang::ASTContext& Ctx) override {
		//std::cout << "HandleTranslationUnit" << std::endl;
		ci.getPreprocessor().getDiagnostics().getClient();

		ASTVisitor v(Ctx.getSourceManager());

		v.TraverseDecl(Ctx.getTranslationUnitDecl());
	}

	virtual bool shouldSkipFunctionBody(clang::Decl *D) override {
		return false;
	}

private:
	clang::CompilerInstance &ci;
};

class ASTAction : public clang::ASTFrontendAction {
protected:
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
			llvm::StringRef InFile) override {
		CI.getFrontendOpts().SkipFunctionBodies = true;

		return std::make_unique<ASTConsumer>(CI);
    }

public:
	bool hasCodeCompletionSupport() const override { return true; }

};


static bool proceedCommand(std::vector<std::string> commands, llvm::StringRef Directory,
                           const std::string &file) {

	std::cout << "file: " << file << std::endl;

	commands = clang::tooling::getClangSyntaxOnlyAdjuster()(commands, file);
	commands = clang::tooling::getClangStripOutputAdjuster()(commands, file);

	for (const auto &com : commands) {
		std::cout << "\t" << com << std::endl;
	}

	clang::FileManager FM({"."});
	FM.Retain();
	clang::tooling::ToolInvocation Inv(commands, new ASTAction(), &FM);

	bool result = Inv.run();
	if (!result) {
		std::cerr << "Error: The file was not recognized as source code: " << file << std::endl;
	}

	return true;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "usage: " << argv[0] << " path to directory with compile_commands.json" << std::endl;
		return 0;
	}

	std::string ErrorMessage;

	std::unique_ptr<clang::tooling::CompilationDatabase> Compilations(
			clang::tooling::FixedCompilationDatabase::loadFromCommandLine(argc, argv, ErrorMessage));
	if (!ErrorMessage.empty()) {
		std::cerr << ErrorMessage << std::endl;
		ErrorMessage = {};
	}

	//	llvm::cl::ParseCommandLineOptions(argc, argv);

	Compilations = std::unique_ptr<clang::tooling::CompilationDatabase>(
			clang::tooling::CompilationDatabase::loadFromDirectory(argv[1], ErrorMessage));

	if (!Compilations && !ErrorMessage.empty()) {
		std::cerr << ErrorMessage << std::endl;
	}

	std::vector<std::string> allFiles = Compilations->getAllFiles();
	std::sort(allFiles.begin(), allFiles.end());

	for (const auto &it: allFiles) {
		std::string file = clang::tooling::getAbsolutePath(it);

		bool isHeader = llvm::StringSwitch<bool>(llvm::sys::path::extension(file))
			.Cases(".h", ".H", ".hh", ".hpp", true)
			.Default(false);

		auto compileCommandsForFile = Compilations->getCompileCommands(file);
		if (!compileCommandsForFile.empty() && !isHeader) {
			proceedCommand(compileCommandsForFile.front().CommandLine,
					compileCommandsForFile.front().Directory, file);
		}
	}
}
