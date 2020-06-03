/*
 ==============================================================================
 
 This file was auto-generated!
 
 It contains the basic startup code for a JUCE application.
 
 ==============================================================================
 */

#include <JuceHeader.h>

void printSection(String message)
{
    DBG( "\n\n=================================================================");
    DBG( message );
    DBG( "=================================================================\n\n");
}

//==============================================================================
int main (int argc, char* argv[])
{
    
    // ..your code goes here!
    auto dataDir = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("PFM_Giveaway_data");
    
    if( !dataDir.exists() )
    {
        DBG( "data dir doesn't exist!" );
        return 1;
    }
    
    printSection("opening customer list");
    /*
     open up the csv
     extract all of the trial customers
     extract all of the full course customers
     eliminate trial customers that bought the full course.
     */
    
    auto customerCSV = dataDir.getChildFile("customers.csv");
    if( !customerCSV.existsAsFile() )
    {
        DBG( "customers.csv doesn't exist!!" );
        return 2;
    }
    
    StringArray customers;
    customerCSV.readLines(customers);
    
    /*
     trial product:
     PFM::C++ For Musicians Day 1-7
     PFM::C++ For Musicians Day 1-7 (old)
     */
    
    /*
     full course:
     PFM::C++ For Musicians Round1
     PFM::C++ For Musicians PRE-ORDER
     PFM::C++ For Musicians Nov 2, 2019
     PFM::C++ For Musicians 2020
     PFM::C++ For Musicians
     */
    customers.remove(0);
    std::unordered_set<String> trialCustomerEmails;
    std::unordered_set<String> fullCourseCustomerEmails;
    
    printSection("adding people who signed up for the trial or full course" );
    for( auto customer : customers )
    {
        /*
         4th column is email
         2nd column is product name
         5th column is do-not-contact
         6th column is purchase date
         */
        auto customerData = StringArray::fromTokens(customer, ",", "\"");
        auto productName = customerData[1];
        if( productName.equalsIgnoreCase("PFM::C++ For Musicians Day 1-7") || productName.equalsIgnoreCase("PFM::C++ For Musicians Day 1-7 (old)"))
        {
            auto customerEmail = customerData[3];
            if( customerData[4] == "1" )
            {
                //DBG( "customer unsubscribed! " << customerEmail );
                continue;
            }
            
            auto purchaseDate = customerData[5];
            if( purchaseDate.contains("2020-06") )
            {
                DBG( "customer signed up after giveaway ended " << customerEmail );
                continue;
            }
            
            if( customerEmail.contains("@") )
            {
                auto [it, success] = trialCustomerEmails.insert(customerEmail);
                if( !success )
                {
                    //DBG( "duplicate signup for trial! " << customerEmail );
                }
            }
        }
        
        if(productName.equalsIgnoreCase("PFM::C++ For Musicians Round1") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians PRE-ORDER") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians Nov 2, 2019") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians 2020") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians") )
        {
            auto customerEmail = customerData[3];
            if( customerData[4] == "1" )
            {
                //                DBG( "customer unsubscribed! " << customerEmail );
                continue;
            }
            if( customerEmail.contains("@") )
            {
                auto [it, success] = fullCourseCustomerEmails.insert(customerEmail);
                if( !success )
                {
                    //                    DBG( "customer purchased the course twice!! " << customerEmail );
                }
            }
        }
    }
    
    DBG( "Trial customers: " << trialCustomerEmails.size() );
    DBG( "full course customers: " << fullCourseCustomerEmails.size() );
    
    printSection("removing full course customers from the list of trial enrollees" );
    
    struct Candidate
    {
        Candidate(String str, int i) : email(str), numEntries(i) { }
        String email, slackID = "NO SLACK ID";
        int numEntries = 1;
        void print() const { DBG( "email: " << email << " numEntries: " << numEntries << " slackID: " << slackID); }
    };
    
    std::vector<Candidate> candidates;
    candidates.reserve(trialCustomerEmails.size());
    
    for( const auto& trialCustomerEmail : trialCustomerEmails )
    {
        bool found = false;
        for( const auto& fullCourseEmail : fullCourseCustomerEmails )
        {
            if( trialCustomerEmail.equalsIgnoreCase(fullCourseEmail) )
            {
                found = true;
                break;
            }
        }
        
        if( !found )
        {
            candidates.emplace_back(trialCustomerEmail, 1);
        }
    }
    
    DBG( "numCandidates: " << candidates.size() );
    
    /*
     parse the Slack stuff
     open the users.json
     */
    auto slackUsers = dataDir.getChildFile("Slack").getChildFile("users.json");
    if( !slackUsers.existsAsFile() )
    {
        DBG( "users.json doesn't exist!!!" );
        return 3;
    }
    
    auto slackUsersJSON = JSON::parse(slackUsers);
    if( !slackUsersJSON.isArray() )
    {
        DBG( "users.json does not contain an array!" );
        return 4;
    }
    
    auto slackUsersArray = *slackUsersJSON.getArray();
    /*
     each element is an object
     object["id"] is what their slack messages will be sent under
     object["profile"]["email"] is their email address
     if they bought the full course, skip them.
     */
    
    struct SlackUser
    {
        String ID, email;
    };
    
    std::vector<SlackUser> slackUsersVector;
    slackUsersVector.reserve(slackUsersArray.size());
    
    for( auto slackUser : slackUsersArray )
    {
        jassert(slackUser.isObject());
        if( !slackUser.isObject() )
        {
            continue;
        }
        
        auto id = slackUser["id"].toString();
        auto email = slackUser["profile"]["email"].toString();
        
        if( email.isEmpty() )
            continue;
        
        bool boughtCourse = false;
        for( const auto& customerEmail : fullCourseCustomerEmails )
        {
            if( customerEmail.equalsIgnoreCase(email) )
            {
                boughtCourse = true;
                DBG( "slack user enrolled in the full course " << customerEmail );
                break;
            }
        }
        
        if( boughtCourse )
            continue;
        
        if( slackUser["deleted"].equals(true) )
        {
            DBG( "user quit slack: " << email );
            continue;
        }
        
        //        DBG( "id: " << id << " email: " << email );
        if( email.equalsIgnoreCase("matkatmusic@gmail.com") || email.equalsIgnoreCase("chordieapp@gmail.com") )
        {
            DBG( "skipping myself!" );
            continue;
        }
        
        SlackUser u;
        u.email = email;
        u.ID = id;
        
        slackUsersVector.push_back(u);
    }
    
    DBG( "num slack users: " << slackUsersVector.size() );
    //    jassertfalse;
    
    printSection("adding an extra entry for candidates who joined slack" );
    
    for( auto& candidate : candidates )
    {
        for( auto& slack : slackUsersVector )
        {
            if( slack.email == candidate.email )
            {
                candidate.slackID = slack.ID;
                ++candidate.numEntries;
                break;
            }
        }
    }
    
    /*
     anyone in slack who is not in candidates gets an entry as long as they didn't buy the course.
     */
    auto candidatesClone = candidates;
    for( auto& slack : slackUsersVector )
    {
        bool found = false;
        for( auto& candidate : candidates )
        {
            if( candidate.email == slack.email )
            {
                found = true;
                break;
            }
        }
        
        if( found )
            continue;
        
        auto boughtCourse = false;
        for( const auto& customerEmail : fullCourseCustomerEmails )
        {
            if( customerEmail.equalsIgnoreCase(slack.email) )
            {
                boughtCourse = true;
                DBG( "slack user enrolled in the full course " << customerEmail );
                break;
            }
        }
        
        if( boughtCourse )
            continue;
        
        Candidate candidate{slack.email, 1};
        candidate.slackID = slack.ID;
        DBG( "adding slack-only candidate: " << candidate.email );
        candidatesClone.push_back(candidate);
        
    }
    
    DBG( "num candidates: " << candidatesClone.size() );
    
    /*
     now parse channel-specific data
     if the slack user shared a commit to a completed project, they get an entry.
     */
    
    /*
     day3-project1
     day4-project2
     day5-project3-part1
     day6-project3-part2
     day7-project4-part1
     
     load json file, extract the array.
     each element is an object
     
     obj["text"] must contain 'github.com' && "Project"
     obj["user"] is their slack ID of the candidate
     */
    
    StringArray dirList;
    dirList.add("day3-project1");
    dirList.add("day4-project2");
    dirList.add("day5-project3-part1");
    dirList.add("day6-project3-part2");
    dirList.add("day7-project4-part1");
    
    printSection("searching through slack data for github submissions");
    
    for( auto folder : dirList )
    {
        auto dayXFolder = dataDir.getChildFile("Slack").getChildFile(folder);
        if( !dayXFolder.exists() )
        {
            DBG( "folder not found! " << dayXFolder.getFullPathName() );
            return 5;
        }
        
        if( dayXFolder.getNumberOfChildFiles(File::findFiles) == 0 )
        {
            DBG( "folder is empty! " << dayXFolder.getFullPathName() );
            continue;
        }
        
        auto it = DirectoryIterator(dayXFolder, false);
        while( it.next() )
        {
            auto jsonFile = it.getFile();
            if( jsonFile.getFileName().contains("2020-06") )
            {
                DBG( "skipping file because it's after the giveaway expiration: " << jsonFile.getFullPathName() );
                continue;
            }
            
            auto json = JSON::parse(jsonFile);
            if( !json.isArray() )
            {
                DBG( "parsing failed for file: " << jsonFile.getFullPathName());
                jassertfalse;
                return 5;
            }
            
            auto& arr = *json.getArray(); //juce::Array<var>
            
            for( auto obj : arr )
            {
                jassert(obj.isObject());
                jassert(obj.hasProperty("text"));
                auto messageText = obj["text"].toString();
                if( messageText.isEmpty() )
                {
                    continue;
                }
                
                if( messageText.contains("github.com") && messageText.contains("PFMCPP_Project") )
                {
                    //                    DBG( "messageText: " << messageText );
                    auto slackUser = obj["user"].toString();
                    for( auto& candidate : candidatesClone )
                    {
                        if( candidate.slackID == slackUser )
                        {
                            if( candidate.email.equalsIgnoreCase("matkatmusic@gmail.com") || candidate.email.equalsIgnoreCase("chordieapp@gmail.com") )
                            {
                                DBG( "skipping messages from myself" );
                                continue;
                            }
                            
                            ++candidate.numEntries;
                            if( candidate.numEntries > 7 )
                            {
                                candidate.numEntries = 7;
                                DBG( "candidate has 7 entries!! " << candidate.email );
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    
    printSection("counting the number of candidates with X entries" );
    std::vector<int> entryCounts { 0, 0, 0, 0, 0, 0, 0, 0 };
    for( auto& candidate : candidatesClone )
    {
        if( candidate.numEntries < entryCounts.size() )
        {
            entryCounts[candidate.numEntries] += 1;
        }
    }
    
    for( int i = 0; i < entryCounts.size(); ++i )
    {
        DBG( "num candidates with " << i << " entries: " << entryCounts[i] );
    }
    
    printSection("creating the final list of candidates based on their number of entries earned");
    
    std::vector<String> finalCandidateList;
    for( auto& candidate : candidatesClone )
    {
        for( int i = 0; i < candidate.numEntries; ++i )
        {
            finalCandidateList.push_back(candidate.email);
        }
    }
    
    DBG("total entries: " << finalCandidateList.size());
    
    printSection("picking a winner!!" );
    
    //    for( int i = 0; i < 10; ++i )
    {
        Random r;
        auto winner = r.nextInt((int)finalCandidateList.size());
        DBG( "" );
        DBG( "winner IDX: " << winner << " email: " << finalCandidateList[winner] );
        auto winnerEmail = finalCandidateList[winner];
        for( auto& candidate : candidatesClone )
        {
            if( candidate.email == winnerEmail )
            {
                DBG( "winner earned " << (candidate.numEntries - 1) << " extra entries to improve their odds" );
            }
        }
        DBG( "" );
        //    }
        
        
        
        return 0;
    }
