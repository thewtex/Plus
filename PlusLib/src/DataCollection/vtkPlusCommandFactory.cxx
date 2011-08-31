
#include "vtkPlusCommandFactory.h"


#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"

#include "vtkPlusCommand.h"
#include "vtkPlusCommandCollection.h"
#include "vtkPlusStartDataCollectionCommand.h"



vtkStandardNewMacro( vtkPlusCommandFactory );



class vtkPlusCommandFactoryCleanup
{
public:
  inline void Use()
    {
    }
  ~vtkPlusCommandFactoryCleanup()
    {
    if( vtkPlusCommandFactory::AvailableCommands )
      {
      vtkPlusCommandFactory::AvailableCommands->Delete();
      vtkPlusCommandFactory::AvailableCommands = 0;
      }
    }
};
static vtkPlusCommandFactoryCleanup vtkPlusCommandFactoryCleanupGlobal;


vtkPlusCommandCollection* vtkPlusCommandFactory::AvailableCommands;



void
vtkPlusCommandFactory
::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Available Commands : ";
  if( AvailableCommands )
    {
    AvailableCommands->PrintSelf( os, indent );
    }
  else
    {
    os << "None.";
    }
}



vtkPlusCommandFactory
::vtkPlusCommandFactory()
{
}



vtkPlusCommandFactory
::~vtkPlusCommandFactory()
{
}



void
vtkPlusCommandFactory
::RegisterCommand( vtkPlusCommand* r)
{
  vtkPlusCommandFactory::InitializeCommands();
  AvailableCommands->AddItem( r );
}



vtkPlusCommand*
vtkPlusCommandFactory
::CreatePlusCommand( std::string str )
{
  vtkPlusCommandFactory::InitializeCommands();
  vtkPlusCommand* ret;
  
  
    // Maybe this section can be deleted. It was taken from vtkImageReader2Factory.
  
  vtkCollection* collection = vtkCollection::New();
  vtkObjectFactory::CreateAllInstance( "vtkPlusCommandObject", collection );
  vtkObject* o;
    // first try the current registered object factories to see if one of them can execute
  for ( collection->InitTraversal(); ( o = collection->GetNextItemAsObject() ); )
    {
    if ( o )
      {
      ret = vtkPlusCommand::SafeDownCast( o );
      if ( ret && ret->CanExecute( str ) )
        {
        return ret;
        }
      }
    }
  collection->Delete();
  
  
  vtkCollectionSimpleIterator sit;
  for( vtkPlusCommandFactory::AvailableCommands->InitTraversal( sit );
      ( ret = vtkPlusCommandFactory::AvailableCommands->GetNextPlusCommand( sit ) ); )
    {
    if( ret->CanExecute( str ) )
      {
      return ret->NewInstance(); // like a new call
      }
    }
  return 0;
}



void
vtkPlusCommandFactory
::InitializeCommands()
{
  if( vtkPlusCommandFactory::AvailableCommands )
    {
    return;
    }
  
  vtkPlusCommandFactoryCleanupGlobal.Use();
  vtkPlusCommandFactory::AvailableCommands = vtkPlusCommandCollection::New();
  vtkPlusCommand* command;

  vtkPlusCommandFactory::AvailableCommands->AddItem( ( command = vtkPlusStartDataCollectionCommand::New() ) );
  command->Delete();
  
}



void
vtkPlusCommandFactory
::GetRegisteredCommands( vtkPlusCommandCollection* collection )
{
  vtkPlusCommandFactory::InitializeCommands();
  vtkObjectFactory::CreateAllInstance( "vtkPlusCommandObject", collection); // get all dynamic readers
  // get the current registered readers
  vtkPlusCommand* ret;
  vtkCollectionSimpleIterator sit;
  for( vtkPlusCommandFactory::AvailableCommands->InitTraversal( sit );
      ( ret = vtkPlusCommandFactory::AvailableCommands->GetNextPlusCommand( sit ));)
    {
    collection->AddItem( ret );
    }
}

