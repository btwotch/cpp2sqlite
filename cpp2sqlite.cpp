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

#include "db.h"

static inline void getFileAndLineNumber(clang::SourceLocation sl, clang::SourceManager &sm, std::string &outFile, int &outLineNumber) {
	const clang::FullSourceLoc fullloc(sl, sm);
	bool invalid = false;
	int retval = fullloc.getSpellingLineNumber(&invalid);
	if (invalid)
		retval = fullloc.getExpansionLineNumber(&invalid);
	if (invalid)
		retval = -1;

	std::string fileName;
	auto fileEntry = fullloc.getFileEntry();
	if (fileEntry == nullptr) {
		fileName = "()";
	} else {
		auto fileNamePtr{fileEntry->getName()};
		if (fileNamePtr.empty())
			fileName = "()";
		fileName = fileNamePtr;
	}

	outFile = fileName;
	outLineNumber = retval;
}

struct ASTVisitor : clang::RecursiveASTVisitor<ASTVisitor> {
	ASTVisitor(clang::SourceManager &sm) : sm(sm) {}

	bool VisitNamespaceDecl(clang::NamespaceDecl *d) {
		std::cout << "namespace: " << d->getQualifiedNameAsString() << std::endl;
		return true;
	}
/*
	bool VisitFieldDecl(clang::FieldDecl *d) {
		std::cout << "field: " << d->getQualifiedNameAsString() << std::endl;
		return true;
	}
*/
	bool VisitFunctionDecl(clang::FunctionDecl *d) {
		clang::SourceLocation sl = d->getBeginLoc();
		if (!sm.isInMainFile(sl)) {
			return true;
		}
		std::string fileName;
		int lineNumber = -1;
		getFileAndLineNumber(sl, sm, fileName, lineNumber);

		std::string functionName;
		std::string className;
		std::string typeName;
		std::string accessSpecifier;
		std::string returnTypeName;
		std::vector<std::pair<std::string, std::string> > args;
		if (clang::CXXConstructorDecl *cxx = llvm::dyn_cast<clang::CXXConstructorDecl>(d)) {
				functionName = "CONSTRUCTOR";
		}
		if (clang::CXXDestructorDecl *cxx = llvm::dyn_cast<clang::CXXDestructorDecl>(d)) {
				functionName = "DESTRUCTOR";
		}
		if (clang::CXXMethodDecl *cxx = llvm::dyn_cast<clang::CXXMethodDecl>(d)) {
			typeName = cxx->getParent()->getKindName();
			className = cxx->getParent()->getName();
			if (cxx->getIdentifier() != nullptr)
				functionName = cxx->getName();
			switch (cxx->getAccess()) {
				case clang::AS_public:
					accessSpecifier = "public";
					break;
				case clang::AS_private:
					accessSpecifier = "private";
					break;
				case clang::AS_protected:
					accessSpecifier = "protected";
					break;
				default:
					break;
			}
			for (unsigned int i = 0; i < d->getNumParams(); i++) {
				clang::ParmVarDecl* PVD = d->getParamDecl(i);
				std::string argTypeName = PVD->getType().getAsString();
				std::string arg = argTypeName;// + " " + PVD->getName();
				args.emplace_back(std::pair<std::string, std::string>{argTypeName, PVD->getName()});
			}

			returnTypeName = d->getDeclaredReturnType().getAsString();
		}
		std::cout << "decl: " << accessSpecifier << " " << returnTypeName << " " << typeName << " " << className << "  " << functionName << " '|";
		for (const std::pair<std::string, std::string> &arg : args) {
			std::cout << arg.first << " " << arg.second << "|";
		}
		std::cout << "': " << fileName << ": " << lineNumber << std::endl;
		return true;
	}

/*
	bool VisitCallExpr(clang::CallExpr *e) {
		clang::SourceLocation slCaller = e->getBeginLoc();
		auto callee = e->getDirectCallee();
		if (callee != nullptr) {
			clang::SourceLocation slCallee = callee->getBeginLoc();
			if (!sm.isInMainFile(slCallee)) {
				return true;
			}
			std::string fileNameCallee;
			int lineNumberCallee = -1;
			getFileAndLineNumber(slCallee, sm, fileNameCallee, lineNumberCallee);
			std::string fileNameCaller;
			int lineNumberCaller = -1;
			getFileAndLineNumber(slCaller, sm, fileNameCaller, lineNumberCaller);
			//std::cout << "\t--> " << callee->getQualifiedNameAsString() << "(" << fileName << ":" << lineNumber << ")" << std::endl;
			std::cout << callerQualifiedName << " --> " << callee->getQualifiedNameAsString() << std::endl;
		}

		return true;
	}
*/

private:
	clang::SourceManager &sm;
	std::string currentNamespace;
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
	ASTAction(DB &db) : db(db) {}

private:
	DB &db;
};


static bool proceedCommand(std::vector<std::string> commands, llvm::StringRef Directory,
                           const std::string &file) {

	commands = clang::tooling::getClangSyntaxOnlyAdjuster()(commands, file);
	commands = clang::tooling::getClangStripOutputAdjuster()(commands, file);

	clang::FileManager FM({"."});
	FM.Retain();
	DB d("test.db");
	clang::tooling::ToolInvocation Inv(commands, new ASTAction(d), &FM);

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
