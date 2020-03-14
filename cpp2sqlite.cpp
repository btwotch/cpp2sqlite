#include <iostream>
#include <memory>
#include <future>
#include <filesystem>
#include <thread>

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
#include "plantuml.h"

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
	ASTVisitor(clang::SourceManager &sm, DB &db) : sm(sm), db(db) {}

	bool VisitVarDecl(clang::VarDecl *d) {
		if (!d->isCXXClassMember()) {
			return true;
		}
		clang::SourceLocation sl = d->getBeginLoc();
		if (!sm.isInMainFile(sl)) {
			return true;
		}
		std::string fileName;
		int lineNumber = -1;
		getFileAndLineNumber(sl, sm, fileName, lineNumber);

		if (clang::CXXRecordDecl *cxx = llvm::dyn_cast<clang::CXXRecordDecl>(d->getDeclContext())) {
			ClassData cd = getClassData(cxx);
			db.addClass(cd);
			std::string varName = d->getName();
			std::string typeName = d->getType().getAsString();
			std::cout << "vardecl: " << typeName << " " << varName << std::endl;
			std::cout << "\tclass: " << cd.className << std::endl;
			VarData vd;
			vd.className = cd.className;
			vd.type = typeName;
			vd.name = varName;
			db.addVarData(vd);
		}

		return true;
	}

	bool VisitFunctionDecl(clang::FunctionDecl *d) {
		clang::SourceLocation sl = d->getBeginLoc();
		if (!sm.isInMainFile(sl)) {
			return true;
		}
		std::string fileName;
		int lineNumber = -1;
		getFileAndLineNumber(sl, sm, fileName, lineNumber);

		std::string functionName;
		ClassData cd;
		std::string typeName;
		std::string accessSpecifier;
		std::string returnTypeName;
		bool isVirtual = false;
		std::vector<FunctionDataArgument> args;
		std::vector<ClassData> bases;
		if (clang::CXXMethodDecl *cxx = llvm::dyn_cast<clang::CXXMethodDecl>(d)) {
			cd = getClassData(cxx->getParent());
			typeName = cxx->getParent()->getKindName();

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
			if (cxx->isVirtualAsWritten()) {
				isVirtual = true;
			}
			for (const auto& base : cxx->getParent()->bases()) {
				ClassData cd = getClassData(base.getType()->getAsCXXRecordDecl());
				bases.emplace_back(cd);
				std::cout << "base " << cd.className << std::endl;
			}
			for (unsigned int i = 0; i < d->getNumParams(); i++) {
				clang::ParmVarDecl* PVD = d->getParamDecl(i);
				FunctionDataArgument arg;
				arg.type = PVD->getType().getAsString();
				arg.name = PVD->getName();
				args.emplace_back(arg);
			}

			returnTypeName = d->getDeclaredReturnType().getAsString();
		}
		if (clang::CXXConstructorDecl *cxx = llvm::dyn_cast<clang::CXXConstructorDecl>(d)) {
				functionName = cxx->getParent()->getName();
		}
		if (clang::CXXDestructorDecl *cxx = llvm::dyn_cast<clang::CXXDestructorDecl>(d)) {
				functionName = std::string{"~"} + std::string{cxx->getParent()->getName()};
		}
		std::string virtualString = isVirtual ? "virtual" : "";
		std::cout << "decl: " << virtualString << " " << accessSpecifier << " " << returnTypeName << " " << typeName << " " << cd.className << " " << functionName << " '|";
		for (const auto &arg : args) {
			std::cout << arg.type << " " << arg.name << "|";
		}
		std::cout << "': " << fileName << ": " << lineNumber << std::endl;

		if (!cd.className.empty()) {
			db.addClass(cd);

			FunctionData fd;
			fd.isVirtual = isVirtual;
			fd.returnTypeName = returnTypeName;
			fd.visibility = accessSpecifier;
			fd.className = cd.className;
			fd.functionName = functionName;
			fd.filepath = fileName;
			fd.lineNumber = lineNumber;
			fd.args = args;
			db.addFunction(fd);

			for (const ClassData &base : bases) {
				db.addInheritance(base.className, cd.className);
			}
		}

		return true;
	}


private:
	clang::SourceManager &sm;
	DB &db;

	ClassData getClassData(clang::CXXRecordDecl *d) {
		clang::SourceLocation sl = d->getBeginLoc();
		ClassData cd;

		cd.className = d->getQualifiedNameAsString();
		getFileAndLineNumber(sl, sm, cd.filepath, cd.lineNumber);

		return cd;
	}
	
};

class ASTConsumer : public clang::ASTConsumer {
public:
	ASTConsumer(clang::CompilerInstance &ci, DB &db) : ci(ci), db(db) {
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

		ASTVisitor v(Ctx.getSourceManager(), db);

		v.TraverseDecl(Ctx.getTranslationUnitDecl());
	}

	virtual bool shouldSkipFunctionBody(clang::Decl *D) override {
		return false;
	}

private:
	clang::CompilerInstance &ci;
	DB &db;
};

class ASTAction : public clang::ASTFrontendAction {
protected:
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
			llvm::StringRef InFile) override {
		CI.getFrontendOpts().SkipFunctionBodies = true;

		return std::make_unique<ASTConsumer>(CI, db);
    }

public:
	bool hasCodeCompletionSupport() const override { return true; }
	ASTAction(DB &db) : db(db) {}

private:
	DB &db;
};


static bool proceedCommand(std::vector<std::string> commands,
                           const std::string &file, DB &db) {

	commands = clang::tooling::getClangSyntaxOnlyAdjuster()(commands, file);
	commands = clang::tooling::getClangStripOutputAdjuster()(commands, file);

	clang::FileManager FM({"."});
	FM.Retain();
	clang::tooling::ToolInvocation Inv(commands, new ASTAction(db), &FM);

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

	DB d("test.db");

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

#if 0
	// TODO: search lock that prevents parallelizing
	std::vector<std::future<bool> > commandFutures;
	for (const auto &it: allFiles) {
		std::string file = clang::tooling::getAbsolutePath(it);

		bool isHeader = llvm::StringSwitch<bool>(llvm::sys::path::extension(file))
			.Cases(".h", ".H", ".hh", ".hpp", true)
			.Default(false);

		auto compileCommandsForFile = Compilations->getCompileCommands(file);
		if (!compileCommandsForFile.empty() && !isHeader) {
			std::vector<std::string> commands = compileCommandsForFile.front().CommandLine;
			std::future<bool> commandFuture = std::async(std::launch::async, [commands, file, &d] {
				return proceedCommand(commands, file, d);
			});
		}

	}

	for (const auto &commandFuture : commandFutures) {
		commandFuture.wait();
	}
#endif
	for (const auto &it: allFiles) {
		std::string file = clang::tooling::getAbsolutePath(it);

		bool isHeader = llvm::StringSwitch<bool>(llvm::sys::path::extension(file))
			.Cases(".h", ".H", ".hh", ".hpp", true)
			.Default(false);

		auto compileCommandsForFile = Compilations->getCompileCommands(file);
		if (!compileCommandsForFile.empty() && !isHeader) {
			std::vector<std::string> commands = compileCommandsForFile.front().CommandLine;
			proceedCommand(commands, file, d);
		}
	}

	auto outputPath = std::filesystem::current_path() / "output";
	PlantumlOutput pu(d, outputPath);
	pu.run();
}
