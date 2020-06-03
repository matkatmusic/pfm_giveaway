/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>

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
    
    /*
     open up the csv
     extract all trial customers
     extract all full course customers.
     eliminate trial customers that bought the full course.
     */
    DBG( "\n\n=========================================================");
    DBG( "opening customer list" );
    DBG( "=========================================================\n\n");
    
    auto customerCSV = dataDir.getChildFile("customers1.csv");
    if(! customerCSV.existsAsFile() )
    {
        DBG( "customers.csv doesn't exist!!" );
        return 2;
    }
    
    StringArray customers;
    customerCSV.readLines(customers);
    
    /*
     Trial product names:
     PFM::C++ For Musicians Day 1-7 (old)
     PFM::C++ For Musicians Day 1-7
     */
    
    /*
     full course names:
     PFM::C++ For Musicians 2020
     PFM::C++ For Musicians
     PFM::C++ For Musicians Nov 2, 2019
     PFM::C++ For Musicians PRE-ORDER
     PFM::C++ For Musicians Round1
     PFM::C++ For Musicians Week 1
     */
    
    customers.remove(0); //column headers
    std::unordered_set<String> trialCustomers;
    std::unordered_set<String> fullCourseCustomerEmails;
    DBG( "\n\n=========================================================");
    DBG( "adding people who signed up for the trial or full course" );
    DBG( "=========================================================\n\n");
    
    for(auto customer : customers )
    {
        auto customerData = StringArray::fromTokens(customer, ",", "\"");
        //4th column is the email address
        //2nd column is the product name.
        auto productName = customerData[1];
        if(productName.equalsIgnoreCase("PFM::C++ For Musicians Day 1-7 (old)") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians Day 1-7"))
        {
            auto customerEmail = customerData[3];
            if( customerEmail.contains("@"))
            {
                auto [it, success] = trialCustomers.insert(customerEmail);
                if( !success)
                    DBG( "duplicate signup for trial! " << customerEmail );
            }
        }
        
        if(productName.equalsIgnoreCase("PFM::C++ For Musicians 2020") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians Nov 2, 2019") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians PRE-ORDER") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians Round1") ||
           productName.equalsIgnoreCase("PFM::C++ For Musicians Week 1"))
        {
            auto customerEmail = customerData[3];
            if( customerEmail.contains("@"))
            {
                auto [it, success] = fullCourseCustomerEmails.insert(customerEmail);
                if( !success )
                    DBG( "customer already added to full course: " << customerEmail << " from product: " << productName );
            }
        }
    }
    
//    DBG( "Trial customers: " << trialCustomers.size() );
//    DBG( "full course customers: " << fullCourseCustomers.size() );
//    jassertfalse;
    
    /*
     remove any emails from trialCustomers that are in fullCourseCustomers
     if you bought the course, why would you enter the giveaway?
     */
    DBG( "\n\n=========================================================");
    DBG( "removing full course customers from list of trial enrollees" );
    DBG( "=========================================================\n\n");
    
    struct Candidate
    {
        Candidate(String str, int i) : email(str), numEntries(i) { }
        String email;
        String slackID = "NO SLACK ID";
        int numEntries = 1;
        
        void print() const { DBG( "email: " << email << " numEntries: " << numEntries << " slackID: " << slackID); }
    };
    
    std::vector<Candidate> candidates;
    candidates.reserve(trialCustomers.size());
    
    for(const auto& trial : trialCustomers )
    {
        bool found = false;
        for(const auto& full : fullCourseCustomerEmails )
        {
            if( trial.equalsIgnoreCase(full) )
            {
                found = true;
                break;
            }
        }
        
        if( !found )
        {
            candidates.emplace_back(trial, 1);
        }
    }
    
    /*
     now parse the slack stuff
     open users.json
     */
    DBG( "\n\n=========================================================");
    DBG( "parsing Slack stuff.  beginning with users.json" );
    DBG( "=========================================================\n\n");
    
    auto slackUsers = dataDir.getChildFile("Slack").getChildFile("users.json");
    if(! slackUsers.existsAsFile() )
    {
        DBG( "users.json doesn't exist!!" );
        return 2;
    }
    
    auto slackUsersJson = JSON::parse(slackUsers);
    if(! slackUsersJson.isArray() )
    {
        DBG( "users.json does not contain an array!" );
        return 2;
    }
    
    auto usersArray = *slackUsersJson.getArray();
    /*
     each element is an object
     object["id"] is what their messages will be sent under
     object["profile"]["email"] is what their user email will be.
     if they bought the full course, skip them.
     */
    struct SlackUser
    {
        String ID, email;
    };
    DBG( "\n\n=========================================================");
    DBG( "creating a Slack Users list" );
    DBG( "=========================================================\n\n");
    
    std::vector<SlackUser> slackUsersVector;
    slackUsersVector.reserve(usersArray.size());
    
    for( auto user : usersArray )
    {
        jassert(user.isObject());
        if(! user.isObject() )
        {
            continue;
        }
        
        auto id = user["id"].toString();
        auto email = user["profile"]["email"].toString();
        
        if( user["deleted"].equals(true) )
        {
            DBG( "user quit slack: " << email );
            continue;
        }
        
        if( email.isEmpty() )
            continue;
        
        bool boughtCourse = false;
        for(const auto& customerEmail : fullCourseCustomerEmails )
        {
            if( customerEmail.equalsIgnoreCase(email) )
            {
                boughtCourse = true;
                DBG( "slack user enrolled in the full course: " << customerEmail );
                break;
            }
        }
        
        if( boughtCourse )
            continue;
        
        DBG( "id: " << id << " email: " << email );
        
        SlackUser u;
        u.email = email;
        u.ID = id;
        
        slackUsersVector.push_back(u);
    }
    
    DBG( "num slack users: " << slackUsersVector.size() );
    /*
     all of the people in SlackUsersVector get one extra entry
     */
    DBG( "\n\n=========================================================");
    DBG( "adding an extra entry for candidates who joined slack" );
    DBG( "=========================================================\n\n");
    
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
    DBG( "\n\n=========================================================");
    DBG( "adding an entry for slack users who are not candidates yet" );
    DBG( "=========================================================\n\n");
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
        
        bool boughtCourse = false;
        for(const auto& email : fullCourseCustomerEmails )
        {
            if( email.equalsIgnoreCase(slack.email) )
            {
                boughtCourse = true;
                DBG( "slack user enrolled in the full course: " << email );
                break;
            }
        }

        if( boughtCourse )
            continue;
        
        Candidate candidate {slack.email, 1};
        candidate.slackID = slack.ID;
        DBG( "adding slack-only candidate: " << candidate.email );
        candidatesClone.push_back(candidate);
    }
    
    DBG( "num Candidates: " << candidatesClone.size() );

    /*
     now parse channel-specific data.
     if the slack user shared a commit to a completed project, they get an entry.
     */
    
    /*
     folder names:
     day3-project1
     day4-project2
     day5-project3-part1
     day6-project3-part2
     day7-project4-part1
     
     load json as array
     each element is an object
     obj["text"] must contain 'github.com'
     obj["user"] is the slack ID of the candidate.
     
     open each directory, open each json file in the directory.
     */
    
    StringArray dirList;
    dirList.add("day3-project1");
    dirList.add("day4-project2");
    dirList.add("day5-project3-part1");
    dirList.add("day6-project3-part2");
    dirList.add("day7-project4-part1");
    
    DBG( "\n\n=========================================================");
    DBG( "searching through Slack data for github submissions" );
    DBG( "=========================================================\n\n");
    for( auto folder : dirList )
    {
        auto dayXFolder = dataDir.getChildFile("Slack").getChildFile(folder);
        if( !dayXFolder.exists() )
        {
            DBG( "folder not found! " );
            return 4;
        }
        if( dayXFolder.getNumberOfChildFiles(File::findFiles) == 0 )
        {
            DBG( "folder is empty!" );
            continue;
        }
        
        auto it = DirectoryIterator(dayXFolder, false);
        while( it.next() )
        {
            auto jsonFile = it.getFile();
            auto json = JSON::parse(jsonFile);
            if( !json.isArray() )
            {
                DBG( "Parse failed for file " << jsonFile.getFileName() );
                return 5;
            }
            
            auto& arr = *json.getArray();
            for( auto obj : arr )
            {
                if( obj.isObject() && obj["text"].toString().contains("github.com") )
                {
                    for( auto& candidate : candidatesClone )
                    {
                        if( candidate.slackID == obj["user"].toString() )
                        {
                            ++candidate.numEntries;
                            if( candidate.numEntries > 7 )
                                candidate.numEntries = 7;
                            if( candidate.numEntries == 7 )
                                DBG( "candidate has 7 entries!! " << candidate.email );
                        }
                    }
                }
            }
        }
    }
    
    DBG( "\n\n=========================================================");
    DBG( "counting the number of candidates with X entries" );
    DBG( "=========================================================\n\n");
    std::vector<int> entryCounts { 0, 0, 0, 0, 0, 0, 0, 0 };
    for( auto& candidate : candidatesClone )
    {
        if( candidate.numEntries < entryCounts.size() )
            ++entryCounts[ candidate.numEntries ];
    }
    
    for( auto i = 0; i < entryCounts.size(); ++i )
    {
        DBG( "num candidates with " << i << " entries: " << entryCounts[i] );
    }
    
    DBG( "\n\n=========================================================");
    DBG( "creating the final list of candidates based on their number of entries earned" );
    DBG( "=========================================================\n\n");
    std::vector<String> finalCandidateList;
    for( auto& candidate : candidatesClone )
    {
        for( int i = 0; i < candidate.numEntries; ++i )
        {
            finalCandidateList.push_back( candidate.email );
        }
    }
    
    DBG( "\n\n=========================================================");
    DBG( "picking the winner!" );
    DBG( "=========================================================\n\n");
    Random r;
    for( int i = 0; i < 10; ++i )
    {
        auto winner = r.nextInt( finalCandidateList.size() );
        DBG( "winner IDX: " << winner << " email: " << finalCandidateList[winner]);
        for( auto& candidate : candidatesClone )
        {
            if( candidate.email == finalCandidateList[winner])
            {
                DBG( "winner earned " << (candidate.numEntries - 1) << " extra entries to improve their odds" );
            }
        }
    }
    
    return 0;
}
